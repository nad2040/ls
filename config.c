#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"

void
argparse(int argc, char *argv[], config_t *config)
{
	int c;
	bool has_set_a;
	opterr = 0;
	while ((c = getopt(argc, argv, "AacdFfhiklnqRrSstuw:")) != -1) {
		switch (c) {
		case 'A':	/* don't show dotdirs */
			if (!has_set_a) {
				config->dots = DOTFILES;
			}
			break;
		case 'a':	/* show dotfiles and dotdirs */
			has_set_a = true;
			config->dots = ALL_DOTS;
			break;
		case 'F':	/* filetype symbol for each filetype, applies in short and long formats */
			SET(config->opts, FILETYPE_SYM);
			break;
		case 'i':	/* show inode */
			SET(config->opts, SHOW_INODES);
			break;
		case 's':	/* show blksize count */
			SET(config->opts, SHOW_BLKCOUNT);
			config->blkcount_fmt = BLKSIZE_ENV;
			break;
		case 'h':	/* show the human readable for both regular size and blksize count */
			config->blkcount_fmt = HUMAN_READABLE;
			break;
		case 'k':	/* display blksize count with size 1kB */
			config->blkcount_fmt = BLKSIZE_KILO;
			break;
			/* long format modifiers */
		case 'l':	/* try to use name instead, fall back to id if missing */
			SET(config->opts, LONG_FORMAT);
			UNSET(config->opts, SHOW_ID_ONLY);
			break;
		case 'n':	/* show id only */
			SET(config->opts, LONG_FORMAT);
			SET(config->opts, SHOW_ID_ONLY);
			break;
			/* recurse modifiers */
		case 'd':
			config->recurse = NO_DEPTH;
			break;
		case 'R':
			config->recurse = FULL_DEPTH;
			break;
			/* sort modifiers */
		case 'S':
			config->sort = SIZE_SORT;
			break;
		case 'r':
			SET(config->opts, REVERSE_SORT);
			break;
		case 'f':	/* no sort - read dirents one by one */
			SET(config->opts, NO_SORT);
			break;
			/* time flags */
		case 't':
			SET(config->opts, TIME_SORT); /* only -t enables sorting by time */
			config->time = MTIME;
			break;
		case 'u':
			config->time = ATIME;
			break;
		case 'c':
			config->time = CTIME;
			break;
			/* non-print chars flags - here we change the default */
		case 'q':
			UNSET(config->opts, RAW_PRINT);
			break;
		case 'w':
			SET(config->opts, RAW_PRINT);
			break;
		case '?':
			if (isprint(optopt)) {
				errx(EXIT_FAILURE, "Unknown option `%c'\n", optopt);
			} else {
				errx(EXIT_FAILURE, "Unknown option `\\x%x'\n", optopt);
			}
		default:
			errx(EXIT_FAILURE, "getopt");
		}
	}
}
