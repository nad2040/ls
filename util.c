#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "ls.h"

extern config_t ls_config;

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
			putchar(c);
		} else if (GET(ls_config.opts, RAW_PRINT)) {
			putchar(c);
		} else {
			putchar('?');
		}
	}
}

#define S_ISEXEC (S_IXUSR | S_IXGRP | S_IXOTH)

void
print_filetype_char(fileinfo_t fileinfo)
{
	switch (fileinfo.mode[0]) {
	case 'l':
		putchar('@');
		break;
	case 'p':
		putchar('|');
		break;
	case 'd':
		putchar('/');
		break;
	default:
		if (GET(fileinfo.statp->st_mode, S_ISEXEC)) {
			putchar('*');
		}
	}
}

/*
 * Compares current time to time tm which is also in local time.
 * returns true if the time is more than 6 months away.
 */
bool
is_older_than_6months(const struct tm tm)
{
	time_t now;
	struct tm *current;

	if ((now = time(NULL)) == -1) {
		err(EXIT_FAILURE, "failed to get time");
	}

	if ((current = localtime(&now)) == NULL) {
		err(EXIT_FAILURE, "failed to acquire localtime");
	}

	return (current->tm_year - tm.tm_year) * 12 +
	           (current->tm_mon - tm.tm_mon) >=
	       6;
}

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
	printf("%s ", buf);
}

/*
 * Prints symlink destination assuming the destination is shorter than PATH_MAX
 */
void
print_symlink_dest(fileinfo_t fileinfo)
{
	int fd;
	ssize_t len;
	char *link_path;
	char link_dest[PATH_MAX];

	printf("\ncwd: %s\n", getcwd(NULL, 0));
	printf("path: %s\n", fileinfo.path);
	printf("accpath: %s\n", fileinfo.accpath);
	printf("name: %s\n", fileinfo.name);

	if ((fd = open(fileinfo.path, O_RDONLY | O_EXCL)) < 0) {
		err(EXIT_FAILURE, "couldn't open fts_path for symlink");
	}


	ASPRINTF("couldn't alloc string for symlink path",
		 &link_path, "%s/%s", fileinfo.path, fileinfo.accpath);
	if ((len = readlinkat(fd, fileinfo.accpath, link_dest, sizeof(link_dest) - 1)) ==
	    -1) {
		err(EXIT_FAILURE, "readlink %s", link_path);
	}

	(void)close(fd);
	link_dest[len] = '\0';

	printf(" -> %s", link_dest);
}

/*
 * Prints the computed fileinfo entries obtained from calling
 * fileinfos_from_ftsents()
 */
void
print_fileinfos(fileinfos_t *fileinfos)
{
	bool long_format, show_inodes, show_blkcount, show_filetype_sym;
	int i;
	fileinfo_t fileinfo;

	long_format = GET(ls_config.opts, LONG_FORMAT);
	show_inodes = GET(ls_config.opts, SHOW_INODES);
	show_blkcount = GET(ls_config.opts, SHOW_BLKCOUNT);
	show_filetype_sym = GET(ls_config.opts, SHOW_FILETYPE_SYM);

	if (long_format && fileinfos->size > 0) {
		printf("total %ld\n", fileinfos->total_blocks);
	}

	for (i = 0; i < fileinfos->size; ++i) {
		fileinfo = fileinfos->arr[i];
		if (show_inodes) {
			printf("%.*ld ", fileinfos->max_inode_len,
			       fileinfo.statp->st_ino);
		}
		if (show_blkcount) {
			printf("%*s ", fileinfos->max_blockcount_len,
			       fileinfo.block_count);
		}
		if (long_format) {
			printf("%s ", fileinfos->arr[i].mode);
			printf("%*d ", fileinfos->max_nlink_len,
			       fileinfo.statp->st_nlink);
			printf("%-*s  ", fileinfos->max_owner_name_or_id_len,
			       fileinfo.owner_name_or_id);
			printf("%-*s  ", fileinfos->max_group_name_or_id_len,
			       fileinfo.group_name_or_id);
			if (fileinfo.use_rdev_nums) {
				printf("%*s%*d, %.*d ",
				       fileinfos->max_size_or_rdev_nums_len -
				           fileinfos->max_rdev_nums_len,
				       "", /* print appropriate padding if the
				            * max length is too short */
				       fileinfos->max_major_len, fileinfo.major,
				       fileinfos->max_minor_len,
				       fileinfo.minor);
			} else {
				printf("%*s ",
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
		putchar('\n');
	}
}

/*
 * allocates a human readable string based on given size.
 * suffixes from SI abbreviations: B K M G T P E Z Y R Q
 * since ANSI doesn't allow long long, only up to exabytes are allowed.
 * TODO: REPLACE FUNCTIONALITY WITH HUMANIZE_NUMBER
 */
char *
human_readable_size_from(unsigned long size)
{
	unsigned long base;
	double div;
	const char *suffix;
	const char *suffixes = "BKMGTPEZYRQ";
	char *human_readable_string;

	suffix = suffixes;

	base = 1;
	while (*suffix != '\0' && (double)size / base / 1024 >= 0.95) {
		base *= 1024;
		suffix++;
	}

	div = (double)size / base;

	if (base > 1 && div < 10) {
		ASPRINTF("failed to alloc human readable string",
		         &human_readable_string, "%.1f%c", div, *suffix);
	} else {
		ASPRINTF("failed to alloc human readable string",
		         &human_readable_string, "%.0f%c", div, *suffix);
	}
	return human_readable_string;
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
	time_t time;
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

		ASPRINTF("couldn't alloc string for accpath",
			 &fileinfo.accpath, "%s", trav->fts_accpath);

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
		file_size = trav->fts_statp->st_size;

		if (ls_config.blkcount_fmt == HUMAN_READABLE) {
			block_count *= 512;
			fileinfo.block_count =
			    human_readable_size_from(block_count);
			fileinfo.file_size =
			    human_readable_size_from(file_size);
		} else {
			block_count = (block_count * 512) / ls_config.blocksize;
			fileinfos->total_blocks += block_count;
			ASPRINTF("couldn't alloc string for block count",
			         &fileinfo.block_count, "%ld", block_count);
			ASPRINTF("couldn't alloc string for file size",
			         &fileinfo.file_size, "%ld", file_size);
		}

		mode = trav->fts_statp->st_mode;

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

		strmode(mode, fileinfo.mode);

		switch (ls_config.time) {
		case ATIME:
			time = fileinfo.statp->st_atime;
			break;
		case MTIME:
			time = fileinfo.statp->st_mtime;
			break;
		case CTIME:
			time = fileinfo.statp->st_ctime;
			break;
		}
		if (localtime_r(&time, &fileinfo.time) == NULL) {
			err(EXIT_FAILURE, "failed to acquire localtime");
		}

		fileinfo.older_than_6months = is_older_than_6months(fileinfo.time);

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
		fileinfos->max_size_or_rdev_nums_len =
		    max(fileinfos->max_size_or_rdev_nums_len,
		        max(fileinfos->max_file_size_len,
		            fileinfos->max_rdev_nums_len));

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
		free(fileinfos->arr[i].accpath);
		free(fileinfos->arr[i].owner_name_or_id);
		free(fileinfos->arr[i].group_name_or_id);
	}
	free(fileinfos->arr);
	free(fileinfos);
}
