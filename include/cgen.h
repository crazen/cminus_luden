#ifndef _CGEN_H_
#define _CGEN_H_

#include "globals.h"
#include "ast.h"


void cGen(TreeNode *t);

/* Função principal de geração de código chamada pelo main */
void codeGen(TreeNode *syntaxTree, char *codefile);

#endif