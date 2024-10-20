#include <sys/stat.h>
#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fts.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef _LS_H_
#define _LS_H_

void print_raw_or_not(const char *);
void ls_print(FTSENT *const, FTSENT *const);

#define F_EXECUTABLE = "*";
#define F_DIRECTORY = "/";
#define F_SYMLINK = "@";
#define F_PIPE = "|";

#define DATE_FORMAT "%b %e "
#define TIME_FORMAT "%H:%M"
#define YEAR_FORMAT "%Y"

typedef struct fileinfo_t {
	char *name;
	char *path;
	struct stat *statp;
	char *owner_name_or_id;
	char *group_name_or_id;
	char *block_count;
	char *file_size;
	devmajor_t major;
	devminor_t minor;
	char mode[12];
	struct tm time;
	bool use_rdev_nums;
	bool older_than_6months;
} fileinfo_t;

#define INIT_CAP 10

typedef struct fileinfos_t {
	fileinfo_t *arr;
	int size;
	int cap;
	int max_inode_len, max_blockcount_len;
	int max_nlink_len;
	int max_owner_name_or_id_len, max_group_name_or_id_len;
	int max_file_size_len, max_rdev_nums_len, max_size_or_rdev_nums_len;
	int max_major_len, max_minor_len;
	int max_time_or_year_len;
} fileinfos_t;

fileinfos_t *fileinfos_from_ftsents(FTSENT *, bool, bool, bool);
void print_fileinfos(fileinfos_t *);
void fileinfos_free(fileinfos_t *);

#define STRDUP(errmsg, newstr, str)                                            \
	do {                                                                   \
		newstr = strdup(str);                                          \
		if (newstr == NULL) {                                          \
			err(EXIT_FAILURE, errmsg);                             \
		}                                                              \
	} while (/* CONSTCOND */ 0)

#define ASPRINTF(errmsg, newstrptr, fmt, ...)                                  \
	do {                                                                   \
		if (asprintf(newstrptr, fmt, __VA_ARGS__) == -1) {             \
			err(EXIT_FAILURE, errmsg);                             \
		}                                                              \
	} while (/* CONSTCOND */ 0)

#endif /* _LS_H_ */
