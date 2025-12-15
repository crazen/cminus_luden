%{
#include "globals.h"
#include "util.h"
#include "scan.h"

/* CORREÇÃO CRÍTICA: Define YYSTYPE explicitamente antes de incluir parse.h */
/* Isso garante que yylval seja tratado como ponteiro, não int */
#define YYSTYPE TreeNode *
#include "parse.h"

char tokenString[256];

void copyTokenString() {
    strncpy(tokenString, yytext, 255);
    tokenString[255] = '\0';
}
%}

%option noyywrap

digit       [0-9]
number      {digit}+
letter      [a-zA-Z_]
identifier  {letter}({letter}|{digit})*
newline     \n
whitespace  [ \t\r]+

%%

"if"         { return IF; }
"else"       { return ELSE; }
"int"        { return INT; }
"return"     { return RETURN; }
"void"       { return VOID; }
"while"      { return WHILE; }
"input"      { return INPUT; }
"output"     { return OUTPUT; }

"+"          { return PLUS; }
"-"          { return MINUS; }
"*"          { return TIMES; }
"/"          { return OVER; }
"<"          { return LT; }
"<="         { return LE; }
">"          { return GT; }
">="         { return GE; }
"=="         { return EQ; }
"!="         { return NE; }
"="          { return ASSIGN; }
";"          { return SEMI; }
","          { return COMMA; }
"("          { return LPAREN; }
")"          { return RPAREN; }
"["          { return LBRACK; }
"]"          { return RBRACK; }
"{"          { return LBRACE; }
"}"          { return RBRACE; }

{number}     { 
               copyTokenString();
               yylval = newExpNode(ConstK);
               yylval->attr.val = atoi(yytext);
               return NUM; 
             }

{identifier} { 
               copyTokenString();
               yylval = newExpNode(IdK);
               yylval->attr.name = copyString(yytext);
               return ID; 
             }

{newline}    { lineno++; }
{whitespace} { /* ignora */ }

"/*" {
    int c;
    while (1) {
        c = input();
        if (c == EOF) break;
        if (c == '\n') lineno++;
        if (c == '*') {
            if ((c = input()) == '/') break;
        }
    }
}

. {
    fprintf(listing, "ERRO LEXICO: '%s' - LINHA: %d\n", yytext, lineno);
    Error = TRUE;
    return ERROR;
}

%%

TokenType getToken(void) {
    static int firstTime = TRUE;
    TokenType currentToken;
    if (firstTime) {
        firstTime = FALSE;
        lineno++;
        yyin = source;
        yyout = listing;
    }
    currentToken = yylex();
    
    if (TraceScan) {
        fprintf(listing,"\t%d: ",lineno);
        printToken(currentToken,tokenString);
    }
    return currentToken;
}