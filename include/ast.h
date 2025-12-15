#ifndef _AST_H_
#define _AST_H_

#include "globals.h"

typedef enum {StmtK, ExpK} NodeKind;
typedef enum {IfK, WhileK, AssignK, ReturnK, CallK, VarDeclK, FunDeclK, CompoundK, ParamK} StmtKind;
typedef enum {OpK, ConstK, IdK, TypeK} ExpKind;

typedef enum {Void, Integer} ExpType;

#define MAXCHILDREN 3

typedef struct treeNode {
    struct treeNode *child[MAXCHILDREN];
    struct treeNode *sibling;
    int lineno;
    NodeKind nodekind;
    union { StmtKind stmt; ExpKind exp; } kind;
    union {
        TokenType op;
        int val;
        char *name;
    } attr;
    ExpType type;
    int memloc;
    int scope; /* 0 = Global, 1 = Local */
} TreeNode;

#endif