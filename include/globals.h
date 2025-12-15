#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define MAXRESERVED 8

/*tokentype - os valores vêm do parse.h gerado pelo Bison */
typedef int TokenType;

extern FILE* source;
extern FILE* listing;
extern FILE* code;

extern int lineno;

extern int EchoSource;
extern int TraceScan;
extern int TraceParse;
extern int TraceAnalyze;
extern int TraceCode;

extern int Error;

#endif