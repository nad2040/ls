#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fts.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LONG_FORMAT (1 << 0)       /* -l or -n flags */
#define SHOW_ID_ONLY (1 << 1)      /* -l or -n flags */
#define SHOW_INODES (1 << 2)       /* -i flag */
#define SHOW_BLKCOUNT (1 << 3)     /* -s flag */
#define SHOW_FILETYPE_SYM (1 << 4) /* -F flag */
#define REVERSE_SORT (1 << 5)      /* -r flag */
#define NO_SORT (1 << 6)           /* -f flag */
#define RAW_PRINT (1 << 7)         /* -q or -w flags */

#define GET(states, bits) ((states & (bits)) != 0)
#define SET(states, bits) states = (states | (bits))
#define UNSET(states, bits) states = (states & ~(bits))

typedef enum dots_opt {
	NO_DOTS,  /* don't show dotfiles or dotdirs */
	DOTFILES, /* -A flag - only show dotfiles and default - default for su
	           */
	ALL_DOTS  /* -a flag - show dotfiles and dotdirs and default - cannot be
	             changed once set */
} dots_opt;

typedef enum recurse_opt {
	NORMAL_DEPTH, /* default maxdepth=1 */
	NO_DEPTH,     /* -d flag maxdepth=0 (i.e. only print .) */
	FULL_DEPTH    /* -R flag no maxdepth */
} recurse_opt;

typedef enum time_opt {
	MTIME, /* -t use last modified time - default */
	ATIME, /* -u use last access time */
	CTIME  /* -c use last status flag change time */
} time_opt;

typedef enum sort_opt {
	LEXICO_SORT, /* default sort - by ascii order */
	TIME_SORT,   /* -t flag - first by specified time_opt. then by ascii
	              */
	SIZE_SORT    /* -S flag - first by size. then by ascii */
} sort_opt;

typedef enum blkcount_fmt_opt {
	BLKSIZE_ENV,  /* -s flag default behavior - number of (getenv(BLOCKSIZE)
	                 or 512)-byte blocks */
	BLKSIZE_KILO, /* -k flag - number of kilobyte blocks (1024) */
	HUMAN_READABLE /* -h flag - num of blocks (rounded) get's converted to a
	                  human readable number. also impacts printing of
	                  regular size */
} blkcount_fmt_opt;

typedef struct config_t {
	uint8_t opts;
	dots_opt dots;
	recurse_opt recurse;
	time_opt time;
	sort_opt sort;
	blkcount_fmt_opt blkcount_fmt;
	blksize_t blocksize;
	int max_depth;
	int (*compare)(const FTSENT **, const FTSENT **);
	bool istty;
} config_t;

void argparse(int *, char ***);

#endif /* _CONFIG_H_ */
