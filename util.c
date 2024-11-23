#include <sys/stat.h>
#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "ls.h"

extern config_t ls_config;

size_t count_digits(size_t);
int max(int, int);
void print_raw_or_not(const char *);
void print_filetype_char(fileinfo_t);
char *human_readable_size_from(size_t, int);
bool is_older_than_6months(const struct timespec);
void print_file_time(fileinfo_t);
void print_symlink_dest(fileinfo_t);
void print_fileinfos(fileinfos_t *);
fileinfos_t *fileinfos_from_ftsents(FTSENT *, bool, bool, bool);
void fileinfos_free(fileinfos_t *);

/*
 * get the number of digits in a number
 */
size_t
count_digits(size_t n)
{
	size_t count;

	if (n == 0) {
		return 1;
	}
	for (count = 0; n > 0; count++, n /= 10) {
		continue;
	}
	return count;
}

/*
 * max of two numbers
 */
int
max(int a, int b)
{
	if (a > b) {
		return a;
	} else {
		return b;
	}
}

/*
 * Prints string based on raw printing config.
 */
void
print_raw_or_not(const char *str)
{
	char c;
	int i;
	for (i = 0; i < (int)strlen(str); ++i) {
		c = str[i];
		if (isprint(c)) {
			(void)putchar(c);
		} else if (GET(ls_config.opts, RAW_PRINT)) {
			(void)putchar(c);
		} else {
			(void)putchar('?');
		}
	}
}

#define F_EXECUTABLE '*'
#define F_DIRECTORY '/'
#define F_SYMLINK '@'
#define F_PIPE '|'
#define S_ISEXEC (S_IXUSR | S_IXGRP | S_IXOTH)

/*
 * Print filetype char for file based on the mode parsed by strmode.
 */
void
print_filetype_char(fileinfo_t fileinfo)
{
	switch (fileinfo.mode[0]) {
	case 'l':
		(void)putchar(F_SYMLINK);
		break;
	case 'p':
		(void)putchar(F_PIPE);
		break;
	case 'd':
		(void)putchar(F_DIRECTORY);
		break;
	default:
		if (GET(fileinfo.statp->st_mode, S_ISEXEC)) {
			(void)putchar(F_EXECUTABLE);
		}
	}
}

/*
 * front end for humanize_number with the same options
 * AUTOSCALE, NOSPACE, and B are forced.
 * returns heap-allocated string of length 5 (including nul).
 */
char *
human_readable_size_from(size_t size, int options)
{
	int flags;
	size_t len;
	char *buf;

	len = 5;
	if ((buf = calloc(1, len)) == NULL) {
		err(EXIT_FAILURE,
		    "failed to allocate string for human readable format");
	}

	flags = HN_NOSPACE | HN_B | options;
	if (humanize_number(buf, len, size, NULL, HN_AUTOSCALE, flags) < 0) {
		err(EXIT_FAILURE, "failed to humanize %ld", size);
	}
	return buf;
}

#define SECONDS_PER_DAY 86400

/*
 * Compares current time to time tm which is also in local time.
 * returns true if the time is more than 6 months away.
 */
bool
is_older_than_6months(const struct timespec tim)
{
	time_t diff;
	struct timespec current_tim;

	if (clock_gettime(CLOCK_REALTIME, &current_tim) < 0) {
		err(EXIT_FAILURE, "couldn't get current time");
	}

	diff = tim.tv_sec - current_tim.tv_sec;
	if (diff < 0) {
		diff = -diff;
	}

	return diff >= 365 * SECONDS_PER_DAY / 2;
}

#define DATE_FORMAT "%b %e "
#define TIME_FORMAT "%H:%M"
#define YEAR_FORMAT "%Y"

/*
 * Returns heap-allocated string with correct time based on config
 * using strftime.
 */
void
print_file_time(fileinfo_t fileinfo)
{
	int total_len;
	size_t size;
	char *buf;

	/* 7 for month and day and spaces + 5 for date/time + 1 for nul */
	total_len = 13;
	buf = calloc(1, total_len);
	if (buf == NULL) {
		err(EXIT_FAILURE, "failed to allocate buffer for strftime");
	}
	if (fileinfo.older_than_6months) {
		size = strftime(buf, total_len, DATE_FORMAT " " YEAR_FORMAT,
		                &fileinfo.time);
	} else {
		size = strftime(buf, total_len, DATE_FORMAT TIME_FORMAT,
		                &fileinfo.time);
	}
	if (size == 0) {
		errx(EXIT_FAILURE, "strftime exceeded buffer");
	}
	(void)printf("%s ", buf);
}

/*
 * Prints symlink destination assuming the destination is shorter than PATH_MAX
 */
void
print_symlink_dest(fileinfo_t fileinfo)
{
	ssize_t len;
	char *link_path;
	char link_dest[PATH_MAX + 1];

	ASPRINTF("couldn't alloc string for symlink path", &link_path, "%s/%s",
	         fileinfo.parent_accpath, fileinfo.name);
	if ((len = readlink(link_path, link_dest, PATH_MAX)) == -1) {
		warn("%s", link_path);
	}
	free(link_path);
	link_dest[len] = '\0';
	(void)printf(" -> %s", link_dest);
}

/*
 * Prints the computed fileinfo entries obtained from calling
 * fileinfos_from_ftsents()
 */
void
print_fileinfos(fileinfos_t *fileinfos)
{
	bool long_format, show_inodes, show_blkcount, show_filetype_sym,
	    human_readable;
	int i;
	fileinfo_t fileinfo;

	long_format = GET(ls_config.opts, LONG_FORMAT);
	show_inodes = GET(ls_config.opts, SHOW_INODES);
	show_blkcount = GET(ls_config.opts, SHOW_BLKCOUNT);
	show_filetype_sym = GET(ls_config.opts, SHOW_FILETYPE_SYM);
	human_readable = (ls_config.blkcount_fmt == HUMAN_READABLE);

	if ((long_format || (show_blkcount && ls_config.istty)) &&
	    fileinfos->size > 0) {
		if (human_readable) {
			(void)printf("total %s\n", human_readable_size_from(
						 fileinfos->total_size, 0));
		} else {
			(void)printf("total %ld\n", (fileinfos->total_blocks * 512 +
			                       ls_config.blocksize - 1) /
			                          ls_config.blocksize);
		}
	}

	for (i = 0; i < fileinfos->size; ++i) {
		fileinfo = fileinfos->arr[i];
		if (show_inodes) {
			(void)printf("%*ld ", fileinfos->max_inode_len,
			       fileinfo.statp->st_ino);
		}
		if (show_blkcount) {
			if (human_readable && !long_format) {
				(void)printf("%*s ", fileinfos->max_file_size_len,
				       fileinfo.file_size);
			} else {
				(void)printf("%*s ", fileinfos->max_blockcount_len,
				       fileinfo.block_count);
			}
		}
		if (long_format) {
			(void)printf("%s ", fileinfos->arr[i].mode);
			(void)printf("%*d ", fileinfos->max_nlink_len,
			       fileinfo.statp->st_nlink);
			(void)printf("%-*s  ", fileinfos->max_owner_name_or_id_len,
			       fileinfo.owner_name_or_id);
			(void)printf("%-*s  ", fileinfos->max_group_name_or_id_len,
			       fileinfo.group_name_or_id);
			if (fileinfo.use_rdev_nums) {
				(void)printf("%*s%*d, %*d ",
				       fileinfos->max_size_or_rdev_nums_len -
				           fileinfos->max_rdev_nums_len,
				       "", /* print appropriate padding if the
				            * max length is too short */
				       fileinfos->max_major_len, fileinfo.major,
				       fileinfos->max_minor_len,
				       fileinfo.minor);
			} else {
				(void)printf("%*s ",
				       fileinfos->max_size_or_rdev_nums_len,
				       fileinfo.file_size);
			}
			print_file_time(fileinfo);
		}
		print_raw_or_not(fileinfo.name);
		if (show_filetype_sym) {
			print_filetype_char(fileinfo);
		}
		if (long_format) {
			if (S_ISLNK(fileinfo.statp->st_mode)) {
				print_symlink_dest(fileinfo);
			}
		}
		(void)putchar('\n');
	}
}

/*
 * creates dynamic array object so the printing widths of the fields can be
 * determined dynamically
 */
fileinfos_t *
fileinfos_from_ftsents(FTSENT *trav, bool non_dir_only, bool dir_only,
                       bool show_warn)
{
	uid_t uid;
	gid_t gid;
	mode_t mode;
	dev_t rdev;
	blkcnt_t block_count;
	off_t file_size;
	struct timespec tim;
	struct passwd *pwd;
	struct group *grp;

	fileinfos_t *fileinfos;
	fileinfo_t fileinfo;

	if ((fileinfos = calloc(1, sizeof(fileinfos_t))) == NULL) {
		err(EXIT_FAILURE, "failed to allocate fileinfos");
	}

	while (trav != NULL) {
		if (trav->fts_errno != 0) {
			if (show_warn) {
				errno = trav->fts_errno;
				warn("%s", trav->fts_name);
			}
			trav = trav->fts_link;
			continue;
		}

		if ((non_dir_only && S_ISDIR(trav->fts_statp->st_mode)) ||
		    (dir_only && !S_ISDIR(trav->fts_statp->st_mode)) ||
		    (ls_config.dots == NO_DOTS && trav->fts_name[0] == '.' &&
		     !non_dir_only && !dir_only)) {
			trav = trav->fts_link;
			continue;
		}

		if (fileinfos->size == fileinfos->cap) {
			fileinfos->cap *= 2;
			if (fileinfos->cap == 0) {
				fileinfos->cap = INIT_CAP;
			}
			fileinfos->arr =
			    realloc(fileinfos->arr,
			            fileinfos->cap * sizeof(fileinfo_t));
			if (fileinfos->arr == NULL) {
				err(EXIT_FAILURE,
				    "failed to realloc dynamic array");
			}
		}

		fileinfo.name = trav->fts_name;
		fileinfo.path = trav->fts_path;
		fileinfo.statp = trav->fts_statp;

		ASPRINTF("couldn't alloc string for parent accpath",
		         &fileinfo.parent_accpath, "%s",
		         trav->fts_parent->fts_accpath);

		uid = trav->fts_statp->st_uid;
		if ((pwd = getpwuid(uid)) == NULL ||
		    GET(ls_config.opts, SHOW_ID_ONLY)) {
			ASPRINTF("couldn't alloc string for owner id",
			         &fileinfo.owner_name_or_id, "%d", uid);
		} else {
			STRDUP("couldn't strdup username",
			       fileinfo.owner_name_or_id, pwd->pw_name);
		}

		gid = trav->fts_statp->st_gid;
		if ((grp = getgrgid(gid)) == NULL ||
		    GET(ls_config.opts, SHOW_ID_ONLY)) {
			ASPRINTF("couldn't alloc string for group id",
			         &fileinfo.group_name_or_id, "%d", gid);
		} else {
			STRDUP("couldn't strdup groupname",
			       fileinfo.group_name_or_id, grp->gr_name);
		}

		block_count = trav->fts_statp->st_blocks;
		fileinfos->total_blocks += block_count;
		file_size = trav->fts_statp->st_size;
		fileinfos->total_size += file_size;

		if (ls_config.blkcount_fmt == HUMAN_READABLE) {
			fileinfo.block_count = human_readable_size_from(
			    block_count * 512, HN_DECIMAL);
			fileinfo.file_size =
			    human_readable_size_from(file_size, HN_DECIMAL);
		} else {
			/* use ceiling */
			block_count =
			    (block_count * 512 + ls_config.blocksize - 1) /
			    ls_config.blocksize;
			ASPRINTF("couldn't alloc string for block count",
			         &fileinfo.block_count, "%ld", block_count);
			ASPRINTF("couldn't alloc string for file size",
			         &fileinfo.file_size, "%ld", file_size);
		}

		mode = trav->fts_statp->st_mode;

		strmode(mode, fileinfo.mode);

		fileinfo.use_rdev_nums =
		    (S_ISCHR(mode) != 0 || S_ISBLK(mode) != 0);

		if (fileinfo.use_rdev_nums) {
			rdev = trav->fts_statp->st_rdev;
			fileinfo.major = major(rdev);
			fileinfo.minor = minor(rdev);
		} else {
			fileinfo.major = 0;
			fileinfo.minor = 0;
		}

		switch (ls_config.time) {
		case ATIME:
			tim = fileinfo.statp->st_atim;
			break;
		case MTIME:
			tim = fileinfo.statp->st_mtim;
			break;
		case CTIME:
			tim = fileinfo.statp->st_ctim;
			break;
		}
		if (localtime_r(&tim.tv_sec, &fileinfo.time) == NULL) {
			err(EXIT_FAILURE, "failed to acquire localtime");
		}

		fileinfo.older_than_6months = is_older_than_6months(tim);

		fileinfos->arr[fileinfos->size++] = fileinfo;

		/* update statistics */
		fileinfos->max_inode_len =
		    max(fileinfos->max_inode_len,
		        count_digits(fileinfo.statp->st_ino));
		fileinfos->max_blockcount_len =
		    max(fileinfos->max_blockcount_len,
		        strlen(fileinfo.block_count));
		fileinfos->max_nlink_len =
		    max(fileinfos->max_nlink_len,
		        count_digits(fileinfo.statp->st_nlink));
		fileinfos->max_owner_name_or_id_len =
		    max(fileinfos->max_owner_name_or_id_len,
		        strlen(fileinfo.owner_name_or_id));
		fileinfos->max_group_name_or_id_len =
		    max(fileinfos->max_group_name_or_id_len,
		        strlen(fileinfo.group_name_or_id));
		fileinfos->max_file_size_len = max(fileinfos->max_file_size_len,
		                                   strlen(fileinfo.file_size));
		fileinfos->max_major_len =
		    max(fileinfos->max_major_len, count_digits(fileinfo.major));
		fileinfos->max_minor_len =
		    max(fileinfos->max_minor_len, count_digits(fileinfo.minor));
		fileinfos->max_rdev_nums_len =
		    max(fileinfos->max_rdev_nums_len,
		        fileinfos->max_major_len + 2 +
		            fileinfos->max_minor_len); /* + 2 for the ", " */
		if (fileinfo.use_rdev_nums) {
			fileinfos->max_size_or_rdev_nums_len =
			    max(fileinfos->max_size_or_rdev_nums_len,
			        fileinfos->max_rdev_nums_len);
		} else {
			fileinfos->max_size_or_rdev_nums_len =
			    max(fileinfos->max_size_or_rdev_nums_len,
			        fileinfos->max_file_size_len);
		}

		if (fileinfo.older_than_6months) {
			fileinfos->max_time_or_year_len =
			    max(fileinfos->max_time_or_year_len,
			        count_digits(1900 + fileinfo.time.tm_year));
		} else {
			fileinfos->max_time_or_year_len =
			    max(fileinfos->max_time_or_year_len, 5);
		}

		trav = trav->fts_link;
	}

	return fileinfos;
}

/*
 * deallocates fileinfos_t, making sure to free all the dynamically allocated
 * strings in the heap
 */
void
fileinfos_free(fileinfos_t *fileinfos)
{
	int i;
	/* Do not free statp or name since it was copied from FTSENT statp */
	for (i = 0; i < fileinfos->size; ++i) {
		free(fileinfos->arr[i].file_size);
		free(fileinfos->arr[i].block_count);
		free(fileinfos->arr[i].parent_accpath);
		free(fileinfos->arr[i].owner_name_or_id);
		free(fileinfos->arr[i].group_name_or_id);
	}
	free(fileinfos->arr);
	free(fileinfos);
}
