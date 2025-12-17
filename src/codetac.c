
#include "globals.h"
#include "codetac.h"
#include "symtab.h"
#include "parse.h"   /* Necessário para PLUS, MINUS, etc. */
#include <stdarg.h>  /* Necessário para va_start, va_end */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tempCount = 0;
static int labelCount = 0;

/* Helpers para gerar nomes */
static char* newTemp() {
    static char buf[20];
    sprintf(buf, "t%d", tempCount++);
    return strdup(buf);
}

static char* newLabel() {
    static char buf[20];
    sprintf(buf, "L%d", labelCount++);
    return strdup(buf);
}

/* Imprime instrução TAC */
static void emitTAC(char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(code, fmt, args);
    fprintf(code, "\n");
    va_end(args);
}

/* Retorna o nome da variável/temp/constante resultante da expressão */
static char* genExp(TreeNode * t) {
    if (t == NULL) return "";

    char *e1, *e2, *t_new;
    char buffer[64];

    switch (t->nodekind) {
        case ExpK:
            switch (t->kind.exp) {
                case ConstK:
                    sprintf(buffer, "%d", t->attr.val);
                    return strdup(buffer);
                
                case IdK:
                    // Se tiver indice (vetor)
                    if (t->child[0] != NULL) {
                        e1 = genExp(t->child[0]); // Indice
                        t_new = newTemp();
                        // Calcula endereco ou carrega valor
                        // Simplificação para TAC: t_new = vetor[indice]
                        emitTAC("%s = %s[%s]", t_new, t->attr.name, e1);
                        return t_new;
                    }
                    return t->attr.name;

                case OpK:
                    e1 = genExp(t->child[0]);
                    e2 = genExp(t->child[1]);
                    t_new = newTemp();
                    
                    switch (t->attr.op) {
                        case PLUS:  emitTAC("%s = %s + %s", t_new, e1, e2); break;
                        case MINUS: emitTAC("%s = %s - %s", t_new, e1, e2); break;
                        case TIMES: emitTAC("%s = %s * %s", t_new, e1, e2); break;
                        case OVER:  emitTAC("%s = %s / %s", t_new, e1, e2); break;
                        case LT:    emitTAC("%s = %s < %s", t_new, e1, e2); break;
                        case LE:    emitTAC("%s = %s <= %s", t_new, e1, e2); break;
                        case GT:    emitTAC("%s = %s > %s", t_new, e1, e2); break;
                        case GE:    emitTAC("%s = %s >= %s", t_new, e1, e2); break;
                        case EQ:    emitTAC("%s = %s == %s", t_new, e1, e2); break;
                        case NE:    emitTAC("%s = %s != %s", t_new, e1, e2); break;
                        default:    emitTAC("%s = %s op %s", t_new, e1, e2); break;
                    }
                    return t_new;
                
                default:
                    return "";
            }
            break;
            
        case StmtK:
            // Caso especial: Chamada de função dentro de expressão
            if (t->kind.stmt == CallK) {
                // Processar argumentos
                TreeNode *arg = t->child[0];
                int n_args = 0;
                // Precisamos empilhar args (em TAC: param x)
                // Vamos percorrer e armazenar para emitir 'param'
                char *argsList[20]; 
                while (arg != NULL) {
                    argsList[n_args++] = genExp(arg);
                    arg = arg->sibling;
                }
                for(int i=0; i<n_args; i++) {
                    emitTAC("param %s", argsList[i]);
                }
                
                t_new = newTemp();
                emitTAC("%s = call %s, %d", t_new, t->attr.name, n_args);
                return t_new;
            }
            break;
            
        default: break;
    }
    return "";
}

static void genStmt(TreeNode * t) {
    if (t == NULL) return;
    
    char *e1, *e2, *L1, *L2; /* Removido L3 que não era usado */

    switch (t->nodekind) {
        case StmtK:
            switch (t->kind.stmt) {
                case IfK:
                    e1 = genExp(t->child[0]); // Condition
                    L1 = newLabel(); // Else ou End
                    L2 = newLabel(); // End (se tiver else)
                    
                    emitTAC("if_false %s goto %s", e1, L1);
                    genStmt(t->child[1]); // Then block
                    
                    if (t->child[2] != NULL) { // Has Else
                        emitTAC("goto %s", L2);
                        emitTAC("label %s", L1);
                        genStmt(t->child[2]); // Else block
                        emitTAC("label %s", L2);
                    } else {
                        emitTAC("label %s", L1);
                    }
                    break;

                case WhileK:
                    L1 = newLabel(); // Start
                    L2 = newLabel(); // End
                    
                    emitTAC("label %s", L1);
                    e1 = genExp(t->child[0]); // Condition
                    emitTAC("if_false %s goto %s", e1, L2);
                    
                    genStmt(t->child[1]); // Body
                    emitTAC("goto %s", L1);
                    emitTAC("label %s", L2);
                    break;

                case AssignK:
                    e2 = genExp(t->child[1]); // Valor
                    TreeNode *varNode = t->child[0];
                    
                    if (varNode->child[0] != NULL) { // Vetor
                        e1 = genExp(varNode->child[0]); // Indice
                        emitTAC("%s[%s] = %s", varNode->attr.name, e1, e2);
                    } else {
                        emitTAC("%s = %s", varNode->attr.name, e2);
                    }
                    break;

                case ReturnK:
                    if (t->child[0] != NULL) {
                        e1 = genExp(t->child[0]);
                        emitTAC("return %s", e1);
                    } else {
                        emitTAC("return");
                    }
                    break;
                
                case CallK:
                    // Chamada como statement (void)
                    {
                        TreeNode *arg = t->child[0];
                        int n_args = 0;
                        char *argsList[20]; 
                        while (arg != NULL) {
                            argsList[n_args++] = genExp(arg);
                            arg = arg->sibling;
                        }
                        for(int i=0; i<n_args; i++) {
                            emitTAC("param %s", argsList[i]);
                        }
                        emitTAC("call %s, %d", t->attr.name, n_args);
                    }
                    break;
                
                case CompoundK:
                    genStmt(t->child[1]); // Executa lista de statements
                    break;
                
                case FunDeclK:
                    emitTAC("\nfunc %s:", t->attr.name);
                    emitTAC("begin_func");
                    genStmt(t->child[2]); // Body
                    emitTAC("end_func");
                    break;

                default: break;
            }
            break;
            
        default: break;
    }
    
    genStmt(t->sibling);
}

void generateTAC(TreeNode * syntaxTree) {
    tempCount = 0;
    labelCount = 0;
    genStmt(syntaxTree);
}
