CC = gcc
CXX = g++
FLEX = flex
BISON = bison
# Note a ordem: -Isrc vem antes ou depois? Tanto faz agora se você deletou include/parse.h
# Mas para garantir, vamos manter ambos.
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -Isrc -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lfl

# Lista de objetos
OBJS = src/main.o src/symtab.o src/analyze.o src/cgen.o src/util.o src/parse.o src/scan.o

all: cminus

# Linkagem final
cminus: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Regra para gerar o analisador léxico
src/scan.c: src/cminus.l src/parse.h
	$(FLEX) -o src/scan.c src/cminus.l

# Regra para gerar o parser e o parse.h (onde estão os defines dos tokens)
src/parse.c src/parse.h: src/cminus.y
	$(BISON) -d -o src/parse.c src/cminus.y

# Regras de compilação genéricas
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# FORÇAR DEPENDÊNCIAS:
# Isso garante que o parse.h seja criado ANTES de compilar esses arquivos
src/cgen.o: src/parse.h
src/analyze.o: src/parse.h
src/util.o: src/parse.h
src/main.o: src/parse.h
src/scan.o: src/scan.c src/parse.h

clean:
	rm -f cminus src/*.o src/scan.c src/parse.h src/parse.c src/*.tm output/*

.PHONY: all clean