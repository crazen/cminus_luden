#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"
#include "ast.h"

void st_insert(char *name, int lineno, int loc, TreeNode *treeNode);
TreeNode* st_lookup(char *name);
void printSymTab(FILE *listing);
void scope_push(char *scopeName);   /* recebe nome */
void scope_pop(void);

#endif
