#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "symtab.h"

#define SIZE 211
#define SHIFT 4

static int hash(char *key) {
    int temp = 0;
    int i = 0;
    while (key[i] != '\0') {
        temp = ((temp << SHIFT) + key[i]) % SIZE;
        ++i;
    }
    return temp;
}

typedef struct BucketListRec {
    char *name;
    TreeNode *treeNode;
    int memloc;
    struct BucketListRec *next;
} *BucketList;

typedef struct ScopeListRec {
    char *name;
    BucketList bucket[SIZE];
    struct ScopeListRec *parent;
    struct ScopeListRec *next_in_all; 
} *Scope;

static Scope scopeStack = NULL;
static Scope globalScope = NULL;

static Scope allScopesHead = NULL;
static Scope allScopesTail = NULL;

void scope_push(char *scopeName) {
    Scope newScope = (Scope) malloc(sizeof(struct ScopeListRec));
    newScope->name = strdup(scopeName);
    newScope->parent = scopeStack;
    newScope->next_in_all = NULL;
    memset(newScope->bucket, 0, sizeof(BucketList)*SIZE);
    
    scopeStack = newScope;
    
    if (globalScope == NULL) globalScope = newScope;
    
    if (allScopesHead == NULL) {
        allScopesHead = newScope;
        allScopesTail = newScope;
    } else {
        allScopesTail->next_in_all = newScope;
        allScopesTail = newScope;
    }
}

void scope_pop(void) {
    if (scopeStack != NULL) {
        scopeStack = scopeStack->parent;
    }
}

void st_insert(char *name, int lineno, int loc, TreeNode *treeNode) {
    (void)lineno;
    int h = hash(name);
    Scope scope = scopeStack ? scopeStack : globalScope;

    BucketList l = scope->bucket[h];
    while (l != NULL) {
        if (strcmp(name, l->name) == 0) {
            return; 
        }
        l = l->next;
    }

    BucketList newEntry = (BucketList) malloc(sizeof(struct BucketListRec));
    newEntry->name = strdup(name);
    newEntry->treeNode = treeNode;
    newEntry->memloc = loc;
    newEntry->next = scope->bucket[h];
    scope->bucket[h] = newEntry;
}

TreeNode* st_lookup(char *name) {
    int h = hash(name);
    Scope scope = scopeStack ? scopeStack : globalScope;

    while (scope != NULL) {
        BucketList l = scope->bucket[h];
        while (l != NULL) {
            if (strcmp(name, l->name) == 0)
                return l->treeNode;
            l = l->next;
        }
        scope = scope->parent;
    }
    return NULL;
}

void printSymTab(FILE *listing) {
    fprintf(listing,"\nTabela de Símbolos (Todos os Escopos):\n");
    fprintf(listing,"Nome        Localização   Linha   Tipo           Escopo\n");
    fprintf(listing,"----------  ------------  ------  -----------    -------\n");

    Scope scope = allScopesHead;
    
    while (scope != NULL) {
        for (int i = 0; i < SIZE; ++i) {
            BucketList l = scope->bucket[i];
            while (l != NULL) {
                TreeNode *node = l->treeNode;
                char *type = "void";
                if (node != NULL) {
                    if (node->type == Integer) type = "int";
                    
                    fprintf(listing,"%-11s %-13d %-7d %-13s %s\n",
                            l->name, l->memloc, node->lineno, type, scope->name);
                }
                l = l->next;
            }
        }
        scope = scope->next_in_all;
    }
}