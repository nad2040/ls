#include <sys/stat.h>
#include <sys/types.h>

#include <grp.h>
#include <pwd.h>
#include <stdlib.h>

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
		if ((fileinfo.ftsent->fts_statp->st_mode &
		     (S_IXUSR | S_IXGRP | S_IXOTH)) > 0) {
			putchar('*');
		}
	}
}

void
print_symlink_dest(fileinfo_t fileinfo)
{
	char link_dest[PATH_MAX];
	ssize_t len;

	if ((len = readlink(fileinfo.ftsent->fts_name, link_dest,
	                    sizeof(link_dest) - 1)) == -1) {
		err(EXIT_FAILURE, "readlink %s", fileinfo.ftsent->fts_name);
	}
	link_dest[len] = '\0';

	printf(" -> %s", link_dest);
}

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

	for (i = 0; i < fileinfos->size; ++i) {
		fileinfo = fileinfos->arr[i];
		if (show_inodes) {
			printf("%.*ld ", fileinfos->max_inode_len,
			       fileinfo.ftsent->fts_ino);
		}
		if (show_blkcount) {
			printf("%*s ", fileinfos->max_blockcount_len,
			       fileinfo.block_count);
		}
		if (long_format) {
			printf("%s  ", fileinfos->arr[i].mode);
			printf("%.*d ", fileinfos->max_nlink_len,
			       fileinfo.ftsent->fts_nlink);
			printf("%-*s  ", fileinfos->max_owner_name_or_id_len,
			       fileinfo.owner_name_or_id);
			printf("%-*s  ", fileinfos->max_group_name_or_id_len,
			       fileinfo.group_name_or_id);
			if (fileinfo.use_rdev_nums) {
				printf("%*s%.*d, %.*d ",
				       fileinfos->max_size_or_rdev_nums_len -
				           fileinfos->max_rdev_nums_len,
				       "", /* print appropriate padding if the
				            * max length is too short */
				       fileinfos->max_major_len,
				       fileinfo.major,
				       fileinfos->max_minor_len,
				       fileinfo.minor);
			} else {
				printf("%*s ",
				       fileinfos->max_size_or_rdev_nums_len,
				       fileinfo.file_size);
			}
			/*
			 * TODO: Print correct time based on config. make a
			 * function also need to use correct timezone.
			 */
		}
		print_raw_or_not(fileinfo.ftsent->fts_name);
		if (show_filetype_sym) {
			print_filetype_char(fileinfo);
		}
		if (long_format) {
			if (S_ISLNK(
				fileinfo.ftsent->fts_statp->st_mode)) {
				print_symlink_dest(fileinfo);
			}
		}
		putchar('\n');
	}
}

/*
 * Prints the children entries of a directory
 * after calling fts_children()
 */
void
ls_print(FTSENT *const parent, FTSENT *const children)
{
	FTSENT *trav;

	trav = children;
	if (GET(ls_config.opts, LONG_FORMAT)) {
		printf("total %ld\n", parent->fts_statp->st_size);
	}

	while (trav != NULL) {
		if (ls_config.dots == NO_DOTS && trav->fts_name[0] == '.') {
			trav = trav->fts_link;
			continue;
		}

		trav = trav->fts_link;
	}
}

/*
 * allocates a human readable string based on given size.
 * suffixes: B K M G T P E
 * only up to exabytes are supported because size_t max is 18 quintillion
 */
char *
human_readable_size_from(size_t size)
{
	char suffix;
	int decimal, num_digits, num_divides, i;
	const char *suffixes = "BKMGTPE";
	char *human_readable_string;

	num_digits = count_digits(size);
	suffix = suffixes[num_digits / 3];

	if (num_digits % 3 == 1 && num_digits > 3) {
		num_divides = num_digits / 3 - 1;
	} else {
		num_divides = num_digits / 3;
	}

	for (i = 0; i < num_divides; ++i) {
		size /= 1000;
	}

	/* do rounding by 75% as done by ls for sizes of length 1 mod 3 */
	if (size >= 1000) {
		if (size % 100 >= 75) {
			size += 100;
		}
		size /= 100;
		decimal = size % 10;
		size /= 10;
		if (asprintf(&human_readable_string, "%ld.%d%c", size, decimal,
		             suffix) == -1) {
			err(EXIT_FAILURE,
			    "failed to alloc human readable string");
		}
	} else {
		if (asprintf(&human_readable_string, "%ld%c", size, suffix) ==
		    -1) {
			err(EXIT_FAILURE,
			    "failed to alloc human readable string");
		}
	}

	return human_readable_string;
}

/*
 * creates dynamic array object so the printing widths of the fields can be
 * determined dynamically
 */
fileinfos_t *
fileinfos_from_ftsents(FTSENT *trav)
{
	uid_t uid;
	gid_t gid;
	mode_t mode;
	dev_t rdev;
	blkcnt_t block_count;
	off_t file_size;
	struct passwd *pwd;
	struct group *grp;

	fileinfos_t *fileinfos;
	fileinfo_t fileinfo;

	if ((fileinfos = calloc(1, sizeof(fileinfos_t))) == NULL) {
		err(EXIT_FAILURE, "failed to allocate fileinfos");
	}

	while (trav != NULL) {
		if (ls_config.dots == NO_DOTS && trav->fts_name[0] == '.') {
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

		fileinfo.ftsent = trav;

		uid = trav->fts_statp->st_uid;
		if ((pwd = getpwuid(uid)) == NULL ||
		    GET(ls_config.opts, SHOW_ID_ONLY)) {
			if (asprintf(&fileinfo.owner_name_or_id, "%d", uid) ==
			    -1) {
				err(EXIT_FAILURE,
				    "couldn't alloc string for owner id");
			}
		} else {
			STRDUP("couldn't strdup username",
			       fileinfo.owner_name_or_id, pwd->pw_name);
		}

		gid = trav->fts_statp->st_gid;
		if ((grp = getgrgid(gid)) == NULL ||
		    GET(ls_config.opts, SHOW_ID_ONLY)) {
			if (asprintf(&fileinfo.group_name_or_id, "%d", gid) ==
			    -1) {
				err(EXIT_FAILURE,
				    "couldn't alloc string for group id");
			}
		} else {
			STRDUP("couldn't strdup groupname",
			       fileinfo.group_name_or_id, grp->gr_name);
		}

		block_count = trav->fts_statp->st_blocks;
		file_size = trav->fts_statp->st_size;

		if (ls_config.blkcount_fmt == HUMAN_READABLE) {
			block_count *= 500;
			fileinfo.block_count =
			    human_readable_size_from(block_count);
			fileinfo.file_size =
			    human_readable_size_from(file_size);
		} else {
			block_count = (block_count * 512) / ls_config.blocksize;
			if (asprintf(&fileinfo.block_count, "%ld",
			             block_count) == -1) {
				err(EXIT_FAILURE,
				    "couldn't alloc string for block count");
			}
			if (asprintf(&fileinfo.file_size, "%ld", file_size) ==
			    -1) {
				err(EXIT_FAILURE,
				    "couldn't alloc string for file size");
			}
		}

		mode = trav->fts_statp->st_mode;

		fileinfo.use_rdev_nums =
		    (S_ISCHR(mode) != 0 || S_ISBLK(mode) != 0);

		if (fileinfo.use_rdev_nums) {
			rdev = trav->fts_statp->st_rdev;
			fileinfo.major = major(rdev);
			fileinfo.minor = minor(rdev);
		}

		strmode(mode, fileinfo.mode);

		fileinfos->arr[fileinfos->size++] = fileinfo;

		/* update statistics */
		fileinfos->max_inode_len =
		    max(fileinfos->max_inode_len, count_digits(trav->fts_ino));
		fileinfos->max_blockcount_len =
		    max(fileinfos->max_blockcount_len,
		        strlen(fileinfo.block_count));
		fileinfos->max_nlink_len = max(fileinfos->max_nlink_len,
		                               count_digits(trav->fts_nlink));
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
	/* Do not free statp since it was copied from FTSENT statp */
	for (i = 0; i < fileinfos->size; ++i) {
		free(fileinfos->arr[i].file_size);
		free(fileinfos->arr[i].block_count);
		free(fileinfos->arr[i].owner_name_or_id);
		free(fileinfos->arr[i].group_name_or_id);
	}
	free(fileinfos->arr);
	free(fileinfos);
}
