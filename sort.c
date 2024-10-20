#include "sort.h"

#include <sys/stat.h>

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
	time_t t1, t2;
	switch (ls_config.sort) {
	case MTIME:
		t1 = (*fts_ent1)->fts_statp->st_mtime;
		t2 = (*fts_ent2)->fts_statp->st_mtime;
		break;
	case ATIME:
		t1 = (*fts_ent1)->fts_statp->st_atime;
		t2 = (*fts_ent2)->fts_statp->st_atime;
		break;
	case CTIME:
		t1 = (*fts_ent1)->fts_statp->st_ctime;
		t2 = (*fts_ent2)->fts_statp->st_ctime;
		break;
	}
	if (t1 == t2) {
		return lexico_sort_func(fts_ent1, fts_ent2);
	}
	if (GET(ls_config.opts, REVERSE_SORT)) {
		return (int)(t2 - t1);
	} else {
		return (int)(t1 - t2);
	}
}

/*
 * Sort fts entries by size and then lexicographically
 */
int
size_sort_func(const FTSENT **fts_ent1, const FTSENT **fts_ent2)
{
	blksize_t s1, s2;
	s1 = (*fts_ent1)->fts_statp->st_size;
	s2 = (*fts_ent2)->fts_statp->st_size;
	if (s1 == s2) {
		return lexico_sort_func(fts_ent1, fts_ent2);
	}
	if (GET(ls_config.opts, REVERSE_SORT)) {
		return s1 - s2;
	} else {
		return s2 - s1;
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
