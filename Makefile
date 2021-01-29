CC=gcc
CCFLAGS=-Wall -Wextra -g

%.o:%.c
	${CC} ${CCFLAGS} -c -o$@ $<

OFILES=main.o
main:${OFILES}
	${CC} ${CCFLAGS} -o$@ ${OFILES}

main.o: main.c
