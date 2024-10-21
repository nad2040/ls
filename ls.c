#include "ls.h"

#include <fts.h>
#include <stdint.h>

#include "config.h"
#include "sort.h"

extern config_t ls_config;

/*
 * Main ls function that uses FTS to traverse the filesystem and
 * return the children nodes at preorder traversal of directories.
 */
int
ls(int argc, char *argv[])
{
	bool did_previously_print;
	bool more_than_one_dir;
	uint8_t exitcode;
	int fts_open_options;
	int ignore_trailing_slash_len;
	char *dot_argv[2];
	char **path_argv;
	FTS *ftsp;
	FTSENT *fs_node;
	FTSENT *children;
	fileinfos_t *fileinfos;
	fileinfos_t *fileinfos_nondir;
	fileinfos_t *fileinfos_dir;

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

	exitcode = EXIT_SUCCESS;

	/* manual doesn't explicitly state NULL is returned, so check errno as
	 * well */
	errno = 0;
	if ((ftsp = fts_open(path_argv, fts_open_options, initial_sort_func)) ==
	        NULL ||
	    errno != 0) {
		exitcode = EXIT_FAILURE;
	}

	did_previously_print = false;
	more_than_one_dir = false;

	children = fts_children(ftsp, 0);
	fileinfos_nondir = fileinfos_from_ftsents(children, true, false, true);
	if (fileinfos_nondir->size > 0) {
		did_previously_print = true;
	}
	print_fileinfos(fileinfos_nondir);
	fileinfos_free(fileinfos_nondir);

	fileinfos_dir = fileinfos_from_ftsents(children, false, true, false);
	if (fileinfos_dir->size > 1) {
		more_than_one_dir = true;
	}
	if (ls_config.recurse == NO_DEPTH) {
		print_fileinfos(fileinfos_dir);
	}
	fileinfos_free(fileinfos_dir);

	ftsp->fts_compar = ls_config.compare;

	while ((fs_node = fts_read(ftsp)) != NULL) {
		if (fs_node->fts_level > ls_config.max_depth ||
		    fs_node->fts_level < 0) {
			fts_set(ftsp, fs_node, FTS_SKIP);
			continue;
		}

		switch (fs_node->fts_info) {
		case FTS_DNR: /* FALLTHROUGH */
		case FTS_ERR: /* FALLTHROUGH */
		case FTS_NS:  /* FALLTHROUGH */
		case FTS_NSOK:
			if (fs_node->fts_level > 0) {
				errno = fs_node->fts_errno;
				warn("%s", fs_node->fts_name);
			}
			exitcode = EXIT_FAILURE;
			break;
		case FTS_D:
			if (ls_config.dots == NO_DOTS &&
			    fs_node->fts_name[0] == '.' &&
			    fs_node->fts_level > 0) {
				fts_set(ftsp, fs_node, FTS_SKIP);
				continue;
			}
			if (did_previously_print) {
				putchar('\n');
			}
			children = fts_children(ftsp, 0);
			if ((ls_config.recurse == FULL_DEPTH &&
			     fs_node->fts_level > 0) ||
			    did_previously_print || more_than_one_dir) {
				/* don't print trailing '/' like ls, unless path
				 * is just '/' */
				ignore_trailing_slash_len =
				    (int)strlen(fs_node->fts_path) - 1;
				if (fs_node->fts_path
				            [ignore_trailing_slash_len] !=
				        '/' ||
				    ignore_trailing_slash_len == 0) {
					ignore_trailing_slash_len++;
				}
				printf("%.*s:\n", ignore_trailing_slash_len,
				       fs_node->fts_path);
			}
			fileinfos = fileinfos_from_ftsents(children, false,
			                                   false, true);
			print_fileinfos(fileinfos);
			fileinfos_free(fileinfos);
			if (!did_previously_print) {
				did_previously_print = true;
			}
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

/*
 * Main function to run ls
 * Parse the args and then run the ls function.
 */
int
main(int argc, char *argv[])
{
	setprogname(argv[0]);

	argparse(&argc, &argv);

	return ls(argc, argv);
}
