#include <sys/types.h>

#include <fts.h>

#ifndef _SORT_H_
#define _SORT_H_

typedef int (*FTSENT_COMPARE)(const FTSENT **, const FTSENT **);

int lexico_sort_func(const FTSENT **, const FTSENT **);
int time_sort_func(const FTSENT **, const FTSENT **);
int size_sort_func(const FTSENT **, const FTSENT **);
int initial_sort_func(const FTSENT **, const FTSENT **);

#endif /* _SORT_H_ */
