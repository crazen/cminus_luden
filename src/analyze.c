
#include "globals.h"
#include "symtab.h"
#include "analyze.h"
#include "ast.h"
#include <string.h>

static int location = 0;
static int currentScope = 0; /* 0 = Global, 1 = Local */
static int global_location = 0;

static void traverse(TreeNode *t, void (*preProc)(TreeNode*), void (*postProc)(TreeNode*)) {
    if (t != NULL) {
        int enterScope = 0;
        if (preProc != NULL) preProc(t);

        if (t->nodekind == StmtK && t->kind.stmt == FunDeclK) {
            if (t->attr.name != NULL) scope_push(t->attr.name);
            else scope_push("func");
            enterScope = 1;
            location = -1; 
            currentScope = 1; 
        }
        else if (t->nodekind == StmtK && t->kind.stmt == CompoundK) {
            scope_push("block");
            enterScope = 1;
        }

        int i;
        for (i=0; i < MAXCHILDREN; i++) {
            if (t->nodekind == ExpK && t->kind.exp == IdK && i == 1) continue;
            traverse(t->child[i], preProc, postProc);
        }

        if (enterScope) {
            scope_pop();
            if (t->nodekind == StmtK && t->kind.stmt == FunDeclK) currentScope = 0; 
        }
        if (postProc != NULL) postProc(t);
        traverse(t->sibling, preProc, postProc);
    }
}

static void insertNode(TreeNode *t) {
    switch (t->nodekind) {
        case StmtK:
            switch (t->kind.stmt) {
                case VarDeclK:
                case ParamK: 
                    {
                        t->scope = currentScope;
                        int size = 1;
                        if (t->kind.stmt == VarDeclK && t->child[0] != NULL && t->child[0]->nodekind == ExpK && t->child[0]->kind.exp == ConstK)
                            size = t->child[0]->attr.val;
                        
                        /* Calcula localizacao antes de inserir */
                        int temp_loc;
                        if (currentScope == 1) { 
                            if (t->kind.stmt == ParamK) location -= 1; 
                            else location -= size; 
                            t->memloc = location; 
                            temp_loc = location;
                        } else { 
                            t->memloc = global_location;
                            temp_loc = global_location;
                            global_location += size;
                        }
                        
                        /* TENTA INSERIR - Se retornar 0, é duplicata! */
                        if (st_insert(t->attr.name, t->lineno, temp_loc, t) == 0) {
                            fprintf(listing, "ERRO SEMANTICO: Redeclaracao da variavel '%s' no mesmo escopo - LINHA: %d\n", t->attr.name, t->lineno);
                            Error = TRUE;
                        }
                    }
                    break;
                case FunDeclK:
                    if (st_insert(t->attr.name, t->lineno, 0, t) == 0) {
                         fprintf(listing, "ERRO SEMANTICO: Redeclaracao da funcao '%s' - LINHA: %d\n", t->attr.name, t->lineno);
                         Error = TRUE;
                    }
                    break;
                case CallK:
                    if (strcmp(t->attr.name, "input") == 0) t->type = Integer;
                    else if (strcmp(t->attr.name, "output") == 0) t->type = Void;
                    else {
                        TreeNode *funcDef = st_lookup(t->attr.name);
                        t->type = (funcDef != NULL) ? funcDef->type : Void;
                    }
                    break;
                default: break;
            }
            break;
        case ExpK:
            if (t->kind.exp == IdK) {
                TreeNode *defn = st_lookup(t->attr.name);
                if (defn != NULL) {
                    t->memloc = defn->memloc;
                    t->type = defn->type;
                    t->scope = defn->scope;
                    if (defn->nodekind == StmtK && defn->kind.stmt == VarDeclK && defn->child[0] != NULL)
                        t->child[1] = defn; 
                    else 
                        t->child[1] = NULL;
                } else if (strcmp(t->attr.name, "input")!=0 && strcmp(t->attr.name, "output")!=0) {
                     fprintf(listing,"ERRO SEMANTICO: Variavel '%s' nao declarada - LINHA: %d\n", t->attr.name, t->lineno);
                     Error = TRUE; t->type = Integer;
                }
            } else if (t->kind.exp == ConstK || t->kind.exp == OpK) {
                t->type = Integer;
            }
            break;
        default: break;
    }
}

void buildSymtab(TreeNode *syntaxTree) {
    location = 0; global_location = 0; currentScope = 0;
    scope_push("global");
    traverse(syntaxTree, insertNode, NULL);
    if (TraceAnalyze && Error == FALSE) printSymTab(listing); /* Só imprime se não tiver erro */
    scope_pop();
}

static void checkNode(TreeNode *t) {
    if (t->nodekind == ExpK) {
        if (t->kind.exp == ConstK || t->kind.exp == OpK) {
            if (t->type == Void) t->type = Integer;
        }
    }
    switch (t->nodekind) {
        case StmtK:
            if (t->kind.stmt == VarDeclK && t->type == Void) {
                fprintf(listing, "ERRO SEMANTICO: Variavel '%s' declarada como void na linha %d\n", t->attr.name, t->lineno);
                Error = TRUE;
            }
            if (t->kind.stmt == AssignK && t->child[1]->type == Void) {
                 fprintf(listing, "ERRO SEMANTICO: Atribuicao de valor void para variavel na linha %d\n", t->lineno);
                 Error = TRUE;
            }
            break;
        case ExpK:
            if (t->kind.exp == OpK) {
                if ((t->child[0]->type == Void) || (t->child[1] && t->child[1]->type == Void)) {
                    fprintf(listing, "ERRO SEMANTICO: Operacao invalida com tipo void na linha %d\n", t->lineno);
                    Error = TRUE;
                }
            }
            break;
        default: break;
    }
}

void typeCheck(TreeNode *syntaxTree) {
    traverse(syntaxTree, NULL, checkNode);
}
