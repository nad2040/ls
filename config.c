#include "config.h"
#include "sort.h"

#include <stdlib.h>

config_t ls_config;

/*
 * Set the default options for the ls_config following the manpage.
 */
void
default_config(void)
{
	int istty;

	ls_config.opts = 0;
	ls_config.dots = NO_DOTS;
	ls_config.recurse = NORMAL_DEPTH;
	ls_config.time = MTIME;
	ls_config.blkcount_fmt = BLKSIZE_ENV;
	ls_config.sort = LEXICO_SORT;

	/* if superuser, -A is always set */
	if (geteuid() == 0) {
		ls_config.dots = DOTFILES;
	}

	/* decide whether to print ? or the raw character depending on the
	 * output filetype */
	errno = 0;
	if ((istty = isatty(STDOUT_FILENO)) == 1) {
		UNSET(ls_config.opts, RAW_PRINT);
	} else if (errno == ENOTTY) {
		SET(ls_config.opts, RAW_PRINT);
	} else {
		err(EXIT_FAILURE, "isatty");
	}
}

/*
 * Prints usage statement for ls.
 */
void
usage(void)
{
	errx(EXIT_FAILURE, "Usage: %s [-AacdFfhiklnqRrSstuw] [file ...]\n",
	     getprogname());
}

#define BLOCKSIZE 512 /* default BLOCKSIZE in struct stat */

/*
 * Get the BLOCKSIZE environment variable.
 */
long
get_blocksize_env()
{
	long blocksize;
	char *blocksize_env, *ep;

	if ((blocksize_env = getenv("BLOCKSIZE")) == NULL) {
		return BLOCKSIZE;
	}
	errno = 0;
	blocksize = strtol(blocksize_env, &ep, 10);
	if (ep == blocksize_env || *ep != '\0') {
		errx(EXIT_FAILURE, "invalid BLOCKSIZE %s", blocksize_env);
	}
	if (errno != 0) {
		err(EXIT_FAILURE, "strtol BLOCKSIZE");
	}
	if (blocksize < 0) {
		errx(EXIT_FAILURE, "negative BLOCKSIZE %ld", blocksize);
	}
	return blocksize;
}

/*
 * Parse the arguments using getopts(3)
 * Pass in pointers to argc and argv directly from main.
 */
void
argparse(int *argc, char ***argv)
{
	bool has_set_a;
	int c;
	long blocksize_env;

	memset(&ls_config, 0, sizeof(config_t));

	default_config();

	blocksize_env = get_blocksize_env();

	opterr = 0;
	while ((c = getopt(*argc, *argv, "AacdFfhiklnqRrSstuw:")) != -1) {
		switch (c) {
		case 'A': /* don't show dotdirs */
			if (!has_set_a) {
				ls_config.dots = DOTFILES;
			}
			break;
		case 'a':                 /* show dotfiles and dotdirs */
			has_set_a = true; /* do not unset once set - behavior
			                     copied from ls(1) */
			ls_config.dots = ALL_DOTS;
			break;
		case 'F': /* filetype symbol for each filetype, applies in short
		             and long formats */
			SET(ls_config.opts, SHOW_FILETYPE_SYM);
			break;
		case 'i': /* show inode */
			SET(ls_config.opts, SHOW_INODES);
			break;
		case 's': /* show blksize count */
			SET(ls_config.opts, SHOW_BLKCOUNT);
			ls_config.blkcount_fmt = BLKSIZE_ENV;
			ls_config.blocksize = blocksize_env;
			break;
		case 'h': /* show the human readable for both regular size and
		             blksize count */
			ls_config.blkcount_fmt = HUMAN_READABLE;
			break;
		case 'k': /* display blksize count with size 1kB */
			ls_config.blkcount_fmt = BLKSIZE_KILO;
			ls_config.blocksize = 1024;
			break;
			/* long format modifiers */
		case 'l': /* try to use name instead, fall back to id if missing
		           */
			SET(ls_config.opts, LONG_FORMAT);
			UNSET(ls_config.opts, SHOW_ID_ONLY);
			break;
		case 'n': /* show id only */
			SET(ls_config.opts, LONG_FORMAT);
			SET(ls_config.opts, SHOW_ID_ONLY);
			break;
			/* recurse modifiers */
		case 'd':
			ls_config.recurse = NO_DEPTH;
			break;
		case 'R':
			ls_config.recurse = FULL_DEPTH;
			break;
			/* sort modifiers */
		case 'r':
			SET(ls_config.opts, REVERSE_SORT);
			break;
		case 'f': /* no sort - read dirents one by one */
			SET(ls_config.opts, NO_SORT);
			/* implied by -f. all entries need to be printed */
			ls_config.dots = ALL_DOTS;
			has_set_a = true;
			break;
		case 'S':
			ls_config.sort = SIZE_SORT;
			break;
			/* time flags */
		case 't':
			ls_config.sort = TIME_SORT; /* only -t enables sorting by time */
			ls_config.time = MTIME;
			break;
		case 'u':
			ls_config.time = ATIME;
			break;
		case 'c':
			ls_config.time = CTIME;
			break;
			/* non-print chars flags - here we change the default */
		case 'q':
			UNSET(ls_config.opts, RAW_PRINT);
			break;
		case 'w':
			SET(ls_config.opts, RAW_PRINT);
			break;
		case '?':
			if (isprint(optopt)) {
				errx(EXIT_FAILURE, "Unknown option `%c'\n",
				     optopt);
			} else {
				errx(EXIT_FAILURE, "Unknown option `\\x%x'\n",
				     optopt);
			}
		default:
			errx(EXIT_FAILURE, "getopt");
		}
	}
	*argc -= optind;
	*argv += optind;

	switch (ls_config.sort) {
	case LEXICO_SORT:
		ls_config.compare = lexico_sort_func;
		break;
	case TIME_SORT:
		ls_config.compare = time_sort_func;
		break;
	case SIZE_SORT:
		ls_config.compare = size_sort_func;
		break;
	}

	/* the -f flag overrides any sorting options. */
	if (GET(ls_config.opts, NO_SORT)) {
		ls_config.compare = NULL;
	}

	switch (ls_config.recurse) {
	case NO_DEPTH:
		ls_config.max_depth = -1;
		break;
	case NORMAL_DEPTH:
		ls_config.max_depth = 0;
		break;
	case FULL_DEPTH:
		ls_config.max_depth = INT_MAX;
		break;
	}
}
