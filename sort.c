#include "config.h"
#include "ls.h"

extern config_t ls_config;

/*
 * sort fts entries lexicographically
 */
int
lexico_sort_func(const FTSENT **fts_ent1, const FTSENT **fts_ent2)
{
	return strcmp((*fts_ent1)->fts_name, (*fts_ent2)->fts_name);
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
	return t1 - t2;
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
	return s2 - s1;
}
