#include <sys/stat.h>
#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <fts.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	printf("Hello world\n");
	return 0;
}
