
#include "globals.h"
#include "util.h"
#include "scan.h"
#include "analyze.h"
#include "cgen.h"
#include "codetac.h" /* Incluindo novo header */
#include <string.h>

TreeNode *parse(void);
extern FILE *yyin;

int lineno = 0;
FILE *source;
FILE *listing;
FILE *code;

int EchoSource = FALSE;
int TraceScan = TRUE;
int TraceParse = TRUE;
int TraceAnalyze = TRUE;
int TraceCode = TRUE;

int Error = FALSE;

void extractFileName(char *path, char *dest) {
    char *filename = strrchr(path, '/');
    if (filename == NULL) filename = path;
    else filename++;
    strcpy(dest, filename);
    char *dot = strrchr(dest, '.');
    if (dot != NULL) *dot = '\0';
}

int main(int argc, char *argv[]) {
    TreeNode *syntaxTree;
    char pgm[120];

    if (argc != 2) {
        fprintf(stderr,"uso: %s <arquivo.c->\n",argv[0]);
        exit(1);
    }

    strcpy(pgm,argv[1]);
    if (strchr(pgm, '.') == NULL) strcat(pgm,".c-");
        
    source = fopen(pgm, "r");
    if (source==NULL) {
        fprintf(stderr,"Arquivo %s não encontrado\n",pgm);
        exit(1);
    }

    yyin = source;
    listing = stdout;

    fprintf(listing,"\nCOMPILAÇÃO C-: %s\n",pgm);

    syntaxTree = parse();
    
    if (!Error) {
        fprintf(listing,"\nÁrvore Sintática:\n");
        printTree(syntaxTree, 0, 0);
    }

    if (!Error) {
        fprintf(listing,"\nConstruindo Tabela de Símbolos...\n");
        buildSymtab(syntaxTree);
    }

    if (!Error) {
        fprintf(listing,"\nChecagem de Tipos...\n");
        typeCheck(syntaxTree);
    }

    if (!Error) {
        char baseName[64];
        char tmfile[128];
        char tacfile[128];
        
        extractFileName(pgm, baseName);
        
        // 1. Gera Assembly TM
        sprintf(tmfile, "output/%s.tm", baseName);
        code = fopen(tmfile,"w");
        if (code == NULL) { printf("Erro ao abrir %s\n", tmfile); exit(1); }
        codeGen(syntaxTree, tmfile);
        fclose(code);

        // 2. Gera Código de 3 Endereços (TAC)
        sprintf(tacfile, "output/%s.tac", baseName);
        code = fopen(tacfile,"w");
        if (code == NULL) { printf("Erro ao abrir %s\n", tacfile); exit(1); }
        
        fprintf(code, "# Código Intermediário (TAC) para %s\n", baseName);
        generateTAC(syntaxTree);
        
        fclose(code);
        printf("Código Intermediário (TAC) gerado em %s\n", tacfile);
    }

    fclose(source);
    return 0;
}
