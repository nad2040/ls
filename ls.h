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

typedef int (*FTSENT_COMPARE)(const FTSENT **, const FTSENT **);

int lexico_sort_func(const FTSENT **, const FTSENT **);
int time_sort_func(const FTSENT **, const FTSENT **);
int size_sort_func(const FTSENT **, const FTSENT **);

void print_raw_or_not(const char *);
void ls_print(FTSENT *const, FTSENT *const);

#define F_EXECUTABLE = "*";
#define F_DIRECTORY = "/";
#define F_SYMLINK = "@";
#define F_PIPE = "|";

#define DATE_FORMAT_NEW "%b %e %H:%M"
#define DATE_FORMAT_OLD "%b %e  %Y"

typedef struct fileinfo_t {
	FTSENT *ftsent;
	char *owner_name_or_id;
	char *group_name_or_id;
	char *block_count;
	char *file_size;
	devmajor_t major;
	devminor_t minor;
	char mode[11];
	bool use_rdev_nums;
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
} fileinfos_t;

fileinfos_t *fileinfos_from_ftsents(FTSENT *);
void print_fileinfos(fileinfos_t *);
void fileinfos_free(fileinfos_t *);

#define STRDUP(errmsg, newstr, str)                                            \
	do {                                                                   \
		newstr = strdup(str);                                          \
		if (newstr == NULL) {                                          \
			err(EXIT_FAILURE, errmsg);                             \
		}                                                              \
	} while (/* CONSTCOND */ 0)

#endif /* _LS_H_ */
