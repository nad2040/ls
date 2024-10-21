#include "sort.h"

#include <sys/stat.h>

#include <string.h>

#include "config.h"

extern config_t ls_config;

/*
 * sort fts entries lexicographically
 */
int
lexico_sort_func(const FTSENT **fts_ent1, const FTSENT **fts_ent2)
{
	int cmp = strcmp((*fts_ent1)->fts_name, (*fts_ent2)->fts_name);
	if (GET(ls_config.opts, REVERSE_SORT)) {
		return -cmp;
	} else {
		return cmp;
	}
}

/*
 * sort fts entries by time and then lexicographically
 */
int
time_sort_func(const FTSENT **fts_ent1, const FTSENT **fts_ent2)
{
	time_t ret;
	struct timespec ts1, ts2;
	switch (ls_config.time) {
	case MTIME:
		ts1 = (*fts_ent1)->fts_statp->st_mtim;
		ts2 = (*fts_ent2)->fts_statp->st_mtim;
		break;
	case ATIME:
		ts1 = (*fts_ent1)->fts_statp->st_atim;
		ts2 = (*fts_ent2)->fts_statp->st_atim;
		break;
	case CTIME:
		ts1 = (*fts_ent1)->fts_statp->st_ctim;
		ts2 = (*fts_ent2)->fts_statp->st_ctim;
		break;
	}
	ret = ts2.tv_sec - ts1.tv_sec;
	if (ret == 0) {
		ret = ts2.tv_nsec - ts1.tv_nsec;
		if (ret == 0) {
			return lexico_sort_func(fts_ent1, fts_ent2);
		}
	}
	if (GET(ls_config.opts, REVERSE_SORT)) {
		return -ret;
	} else {
		return ret;
	}
}

/*
 * Sort fts entries by size and then lexicographically
 */
int
size_sort_func(const FTSENT **fts_ent1, const FTSENT **fts_ent2)
{
	int ret;
	blksize_t s1, s2;
	s1 = (*fts_ent1)->fts_statp->st_size;
	s2 = (*fts_ent2)->fts_statp->st_size;
	ret = s2 - s1;
	if (ret == 0) {
		return lexico_sort_func(fts_ent1, fts_ent2);
	}
	if (GET(ls_config.opts, REVERSE_SORT)) {
		return -ret;
	} else {
		return ret;
	}
}

/*
 * Initially sort by directory
 */
int
initial_sort_func(const FTSENT **fts_ent1, const FTSENT **fts_ent2)
{
	int e1, e2;
	if (S_ISDIR((*fts_ent1)->fts_statp->st_mode)) {
		e1 = 1;
	} else {
		e1 = 0;
	}
	if (S_ISDIR((*fts_ent2)->fts_statp->st_mode)) {
		e2 = 1;
	} else {
		e2 = 0;
	}
	if (e1 == e2 && ls_config.compare != NULL) {
		return ls_config.compare(fts_ent1, fts_ent2);
	}
	return e1 - e2;
}
