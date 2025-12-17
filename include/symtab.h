
#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"
#include "ast.h"

/* Retorna 1 se inseriu com sucesso, 0 se já existia no escopo atual */
int st_insert(char *name, int lineno, int loc, TreeNode *treeNode);

TreeNode* st_lookup(char *name);
void printSymTab(FILE *listing);
void scope_push(char *scopeName);
void scope_pop(void);

#endif
