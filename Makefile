CC=gcc
CFLAGS=-ansi -Wall -Werror -Wextra -Wpedantic -Wshadow -g -Wformat=2 -Wjump-misses-init -Wlogical-op
PROG=ls
OBJS=ls.o config.o sort.o util.o

all: ${PROG}

${PROG}: ${OBJS}
	${CC} ${OBJS} -o ${PROG} ${LDFLAGS}

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

clean:
	rm -f ${PROG} *.o

depend:
	mkdep -- ${CFLAGS} *.c

tags:
	ctags *.c *.h
