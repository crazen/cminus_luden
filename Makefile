
CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -Isrc -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lfl


OBJS = src/main.o src/symtab.o src/analyze.o src/cgen.o src/codetac.o src/util.o src/parse.o src/scan.o

all: cminus

cminus: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

src/scan.c: src/cminus.l src/parse.h
	$(FLEX) -o src/scan.c src/cminus.l

src/parse.c src/parse.h: src/cminus.y
	$(BISON) -d -o src/parse.c src/cminus.y

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencias
src/cgen.o: src/parse.h
src/codetac.o: src/parse.h
src/analyze.o: src/parse.h
src/util.o: src/parse.h
src/main.o: src/parse.h
src/scan.o: src/scan.c src/parse.h

clean:
	rm -f cminus src/*.o src/scan.c src/parse.h src/parse.c src/*.tm output/* output/*.tac

.PHONY: all clean
