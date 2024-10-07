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

/*
 * Prints string based on raw printing config.
 */
void
print_raw_or_not(const char *str) {
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
 * Prints the children entries of a directory
 * after calling fts_children()
 */
void
ls_print(FTSENT * const parent, FTSENT * const children)
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

		print_raw_or_not(trav->fts_name);
		putchar('\n');
		trav = trav->fts_link;
	}
}

int
ls(int argc, char *argv[])
{
	int fts_open_options;
	int max_depth;
	int ignore_trailing_slash_len;
	char *dot_argv[2];
	char **path_argv;
	int (*sort_func)(const FTSENT **, const FTSENT **);
	FTS *ftsp;
	FTSENT *fs_node;
	FTSENT *children;

	if (argc == 0) {
		dot_argv[0] = ".";
		dot_argv[1] = NULL;
		path_argv = dot_argv;
	} else {
		path_argv = argv;
	}

	fts_open_options = FTS_PHYSICAL;
	if (ls_config.dots == ALL_DOTS) {
		fts_open_options |= FTS_SEEDOT;
	}

	switch (ls_config.sort) {
	case LEXICO_SORT:
		sort_func = lexico_sort_func;
		break;
	case TIME_SORT:
		sort_func = time_sort_func;
		break;
	case SIZE_SORT:
		sort_func = size_sort_func;
		break;
	default:
		break;
	}

	/* the -f flag overrides any sorting options. */
	if (GET(ls_config.opts, NO_SORT)) {
		sort_func = NULL;
	}

	switch (ls_config.recurse) {
	case NO_DEPTH:
		max_depth = -1;
		break;
	case NORMAL_DEPTH:
		max_depth = 0;
		break;
	case FULL_DEPTH:
		max_depth = INT_MAX;
		break;
	default:
		break;
	}

	/* manual doesn't explicitly state NULL is returned, so check errno as well */
	errno = 0;
	if ((ftsp = fts_open(path_argv, fts_open_options, sort_func)) == NULL || errno != 0) {
		printf("ftsp = %p\nerrno = %d\n",(void*)ftsp,errno);
		err(EXIT_FAILURE, "fts_open");
	}

	while ((fs_node = fts_read(ftsp)) != NULL) {
		if (fs_node->fts_level > max_depth || fs_node->fts_level < 0) {
			/* printf("Skipping %s at level %d\n", fs_node->fts_path, fs_node->fts_level); */
			continue;
		}

		switch (fs_node->fts_info) {
		case FTS_DNR: /* FALLTHROUGH */
		case FTS_ERR: /* FALLTHROUGH */
		case FTS_NS: /* FALLTHROUGH */
			errno = fs_node->fts_errno;
			warn("fts_read");
			break;
		case FTS_D:
			children = fts_children(ftsp, 0);
			if (ls_config.recurse == FULL_DEPTH && fs_node->fts_level > 0) {
				/* don't print trailing '/' like ls */
				ignore_trailing_slash_len = (int) strlen(fs_node->fts_path) - 1;
				if (fs_node->fts_path[ignore_trailing_slash_len] != '/') {
					ignore_trailing_slash_len++;
				}
				printf("\n%.*s:\n", ignore_trailing_slash_len, fs_node->fts_path);
			}
			ls_print(fs_node, children);
			break;
		case FTS_DP: /* FALLTHROUGH */
		case FTS_DOT:
			break;
		case FTS_F: /* FALLTHROUGH */
		case FTS_SL: /* FALLTHROUGH */
			/* printf("%s\n", fs_node->fts_name); */
			break;
		default:
			break;
		}
	}
	if (errno != 0) {
		err(EXIT_FAILURE, "fts_read");
	}

	if (fts_close(ftsp) < 0) {
		err(EXIT_FAILURE, "fts_close");
	}

	return 0;
}


int
main(int argc, char *argv[])
{
	setprogname(argv[0]);

	argparse(&argc, &argv);

	(void)ls(argc, argv);

	return 0;
}
