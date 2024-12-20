#include <sys/types.h>

#include <err.h>
#include <fts.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _LS_H_
#define _LS_H_

typedef struct fileinfo_t {
	char *name;
	char *path;
	char *parent_accpath;
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
	blkcnt_t total_blocks;
	size_t total_size;
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
