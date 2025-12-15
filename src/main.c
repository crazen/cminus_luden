/* main.c - Versão com Saída Dinâmica */
#include "globals.h"
#include "util.h"
#include "scan.h"
#include "analyze.h"
#include "cgen.h"
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

/* Função auxiliar para extrair o nome base do arquivo 
   Ex: "tests/gcd.c-" -> "gcd" */
void extractFileName(char *path, char *dest) {
    char *filename = strrchr(path, '/');
    if (filename == NULL) {
        filename = path; /* Não tem barra, usa o próprio nome */
    } else {
        filename++; /* Pula a barra */
    }
    
    strcpy(dest, filename);
    
    /* Remove a extensão (o último ponto) */
    char *dot = strrchr(dest, '.');
    if (dot != NULL) {
        *dot = '\0';
    }
}

int main(int argc, char *argv[]) {
    TreeNode *syntaxTree;
    char pgm[120];

    if (argc != 2) {
        fprintf(stderr,"uso: %s <arquivo.c->\n",argv[0]);
        exit(1);
    }

    strcpy(pgm,argv[1]);
    if (strchr(pgm, '.') == NULL)
        strcat(pgm,".c-");
        
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
        char codefile[128];
        
        /* Extrai "teste" de "tests/teste.c-" */
        extractFileName(pgm, baseName);
        
        /* Monta "output/teste.tm" */
        sprintf(codefile, "output/%s.tm", baseName);
        
        code = fopen(codefile,"w");
        if (code == NULL) {
            printf("Não foi possível abrir %s para escrita\n",codefile);
            exit(1);
        }
        codeGen(syntaxTree, codefile);
        fclose(code);
    }

    fclose(source);
    return 0;
}