#include "ls.h"

#include <fts.h>
#include <stdint.h>

#include "config.h"


extern config_t ls_config;

int
ls(int argc, char *argv[])
{
	uint8_t exitcode;
	int fts_open_options;
	int max_depth;
	int ignore_trailing_slash_len;
	char *dot_argv[2];
	char **path_argv;
	FTSENT_COMPARE sort_func;
	FTS *ftsp;
	FTSENT *fs_node;
	FTSENT *children;
	fileinfos_t *fileinfos;

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
	}

	exitcode = EXIT_SUCCESS;

	/* TODO:
	 * Fix the sort function for the first iteration to first sort by directory
	 * or non-directory, and then by the sort function determined by config.
	 * refactor sort_func and max_depth into config struct
	 */

	/* manual doesn't explicitly state NULL is returned, so check errno as
	 * well */
	errno = 0;
	if ((ftsp = fts_open(path_argv, fts_open_options, sort_func)) == NULL ||
	    errno != 0) {
		printf("ftsp = %p\nerrno = %d\n", (void *)ftsp, errno);
		err(EXIT_FAILURE, "fts_open");
	}

	while ((fs_node = fts_read(ftsp)) != NULL) {
		if (fs_node->fts_level > max_depth || fs_node->fts_level < 0) {
			/* printf("Skipping %s at level %d\n",
			 * fs_node->fts_path, fs_node->fts_level); */

			fts_set(ftsp, fs_node, FTS_SKIP);
			continue;
		}

		switch (fs_node->fts_info) {
		case FTS_DNR: /* FALLTHROUGH */
		case FTS_ERR: /* FALLTHROUGH */
		case FTS_NS:  /* FALLTHROUGH */
		case FTS_NSOK:
			errno = fs_node->fts_errno;
			warn("%s", fs_node->fts_name);
			exitcode = EXIT_FAILURE;
			break;
		case FTS_D:
			children = fts_children(ftsp, 0);
			if (ls_config.recurse == FULL_DEPTH &&
			    fs_node->fts_level > 0) {
				/* don't print trailing '/' like ls */
				ignore_trailing_slash_len =
				    (int)strlen(fs_node->fts_path) - 1;
				if (fs_node
				        ->fts_path[ignore_trailing_slash_len] !=
				    '/') {
					ignore_trailing_slash_len++;
				}
				printf("\n%.*s:\n", ignore_trailing_slash_len,
				       fs_node->fts_path);
			}
			fileinfos = fileinfos_from_ftsents(children);
			print_fileinfos(fileinfos);
			fileinfos_free(fileinfos);
			break;
		case FTS_F: /* FALLTHROUGH */
		case FTS_SL:
			fts_set(ftsp, fs_node, FTS_SKIP);
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

	return exitcode;
}

int
main(int argc, char *argv[])
{
	setprogname(argv[0]);

	argparse(&argc, &argv);

	(void)ls(argc, argv);

	return 0;
}
