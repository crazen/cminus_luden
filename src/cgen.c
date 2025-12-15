#include "globals.h"
#include "symtab.h"
#include "cgen.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AC 0
#define AC1 1
#define GP 5
#define MP 6
#define PC 7

static int tmpOffset = 0;
static int emitLoc = 0; 
static int highEmitLoc = 0; 

typedef struct FuncRec { char *name; int address; struct FuncRec *next; } *FuncList;
typedef struct CallRec { int loc; char *name; struct CallRec *next; } *CallList;
static FuncList funcTable = NULL;
static CallList callList = NULL;

void addFunction(char *name, int addr) {
    FuncList f = (FuncList)malloc(sizeof(struct FuncRec));
    f->name = strdup(name); f->address = addr; f->next = funcTable; funcTable = f;
}
int getFunctionAddr(char *name) {
    FuncList f = funcTable;
    while(f!=NULL) { if(strcmp(f->name, name)==0) return f->address; f=f->next; }
    return -1;
}
void addCallPatch(int loc, char *name) {
    CallList c = (CallList)malloc(sizeof(struct CallRec));
    c->loc = loc; c->name = strdup(name); c->next = callList; callList = c;
}

static void emitComment(char *c) { if (TraceCode) fprintf(code, "* %s\n", c); }
static void emitRO(char *op, int r, int s, int t, char *c) {
    fprintf(code, "%3d:  %5s  %d,%d,%d    %s\n", emitLoc++, op, r, s, t, c);
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}
static void emitRM(char *op, int r, int d, int s, char *c) {
    fprintf(code, "%3d:  %5s  %d,%d(%d)    %s\n", emitLoc++, op, r, d, s, c);
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}
static int emitSkip(int howMany) { int i = emitLoc; emitLoc += howMany; if (highEmitLoc < emitLoc) highEmitLoc = emitLoc; return i; }
static void emitBackup(int loc) { emitLoc = loc; }
static void emitRestore(void) { emitLoc = highEmitLoc; }
static void emitRM_Abs(char *op, int r, int a, char *c) {
    fprintf(code, "%3d:  %5s  %d,%d(%d)    %s\n", emitLoc, op, r, a - (emitLoc + 1), 7, c);
    ++emitLoc;
    if (highEmitLoc < emitLoc) highEmitLoc = emitLoc;
}

void cGen(TreeNode *t) {
    int savedLoc1, savedLoc2, currentLoc;
    if (t == NULL) return;
    
    switch (t->nodekind) {
        case StmtK:
            switch (t->kind.stmt) {
                case FunDeclK:
                    emitComment("-> Inicio Funcao");
                    addFunction(t->attr.name, emitLoc);
                    tmpOffset = -20; 
                    emitRM("ST", AC, -1, MP, "store return addr");
                    cGen(t->child[2]);
                    emitRM("LD", PC, -1, MP, "return");
                    emitComment("<- Fim Funcao");
                    break;
                case ReturnK:
                    cGen(t->child[0]); emitRM("LD", PC, -1, MP, "return");
                    break;
                case CallK:
                    if (strcmp(t->attr.name, "input") == 0) {
                        emitRO("IN", AC, 0, 0, "read int");
                    } else if (strcmp(t->attr.name, "output") == 0) {
                        cGen(t->child[0]); emitRO("OUT", AC, 0, 0, "write int");
                    } else {
                        TreeNode *arg = t->child[0];
                        int argIdx = 0;
                        int frameBase = tmpOffset; 
                        
                        while (arg != NULL) {
                            cGen(arg);
                            emitRM("ST", AC, frameBase - 2 - argIdx, MP, "push arg");
                            arg = arg->sibling;
                            argIdx++;
                        }
                        emitRM("ST", MP, frameBase, MP, "store MP"); 
                        emitRM("LDA", MP, frameBase, MP, "new frame");
                        emitRM("LDC", AC, emitLoc + 2, 0, "ret addr");
                        int jumpLoc = emitSkip(1);
                        addCallPatch(jumpLoc, t->attr.name);
                        emitRM("LD", MP, 0, MP, "pop frame");
                    }
                    break;
                case CompoundK:
                    cGen(t->child[0]); cGen(t->child[1]);
                    break;
                case AssignK:
                    if (t->child[0]->child[0] != NULL) {
                        /* Array Index Assignment */
                        cGen(t->child[0]->child[0]); 
                        emitRM("ST", AC, tmpOffset--, MP, "save index");
                        cGen(t->child[1]); 
                        emitRM("LD", AC1, ++tmpOffset, MP, "restore index");
                        emitRM("ST", AC, tmpOffset--, MP, "save value");
                        
                        if (t->child[0]->scope == 0) {
                            emitRM("LDC", AC, t->child[0]->memloc, 0, "base global");
                            emitRO("ADD", AC, AC, GP, "global addr");
                        } else {
                            if (t->child[0]->child[1] != NULL) {
                                emitRM("LDC", AC, t->child[0]->memloc, 0, "local offset");
                                emitRO("ADD", AC, AC, MP, "local addr");
                            } else {
                                emitRM("LD", AC, t->child[0]->memloc, MP, "load ptr");
                            }
                        }
                        emitRO("ADD", AC1, AC1, AC, "addr");
                        emitRM("LD", AC, ++tmpOffset, MP, "restore value");
                        emitRM("ST", AC, 0, AC1, "store array");
                    } else {
                        cGen(t->child[1]);
                        if (t->child[0]->scope == 0) emitRM("ST", AC, t->child[0]->memloc, GP, "global");
                        else emitRM("ST", AC, t->child[0]->memloc, MP, "local");
                    }
                    break;
                case IfK:
                    cGen(t->child[0]);
                    savedLoc1 = emitSkip(1); cGen(t->child[1]);
                    savedLoc2 = emitSkip(1); currentLoc = emitSkip(0);
                    emitBackup(savedLoc1); emitRM_Abs("JEQ", AC, currentLoc, "jmp false"); emitRestore();
                    cGen(t->child[2]); currentLoc = emitSkip(0);
                    emitBackup(savedLoc2); emitRM_Abs("LDA", PC, currentLoc, "jmp end"); emitRestore();
                    break;
                case WhileK:
                    savedLoc1 = emitSkip(0); cGen(t->child[0]); savedLoc2 = emitSkip(1); cGen(t->child[1]);
                    emitRM_Abs("LDA", PC, savedLoc1, "jmp loop"); currentLoc = emitSkip(0);
                    emitBackup(savedLoc2); emitRM_Abs("JEQ", AC, currentLoc, "jmp exit"); emitRestore();
                    break;
                default: break;
            }
            break;
        case ExpK:
            switch (t->kind.exp) {
                case ConstK: emitRM("LDC", AC, t->attr.val, 0, "const"); break;
                case IdK:
                    if (t->child[0] != NULL) { /* Array Access */
                        cGen(t->child[0]); emitRM("LDC", AC1, 0, 0, "clear"); emitRO("ADD", AC1, AC, AC1, "move index");
                        if (t->scope == 0) {
                            emitRM("LDC", AC, t->memloc, 0, "base global"); emitRO("ADD", AC, AC, GP, "global addr");
                        } else {
                            if (t->child[1] != NULL) {
                                emitRM("LDC", AC, t->memloc, 0, "base local"); emitRO("ADD", AC, AC, MP, "local addr");
                            } else {
                                emitRM("LD", AC, t->memloc, MP, "load ptr");
                            }
                        }
                        emitRO("ADD", AC1, AC1, AC, "addr"); emitRM("LD", AC, 0, AC1, "load array");
                    } else {
                        if (t->scope == 0) {
                            if (t->child[1] != NULL) emitRM("LDA", AC, t->memloc, GP, "load global addr");
                            else emitRM("LD", AC, t->memloc, GP, "load global val");
                        } else {
                            if (t->child[1] != NULL) emitRM("LDA", AC, t->memloc, MP, "load local addr");
                            else emitRM("LD", AC, t->memloc, MP, "load local val");
                        }
                    }
                    break;
                case OpK:
                    cGen(t->child[0]); emitRM("ST", AC, tmpOffset--, MP, "push left");
                    cGen(t->child[1]); emitRM("LD", AC1, ++tmpOffset, MP, "load left");
                    switch (t->attr.op) {
                        case PLUS: emitRO("ADD", AC, AC1, AC, "op +"); break;
                        case MINUS: emitRO("SUB", AC, AC1, AC, "op -"); break;
                        case TIMES: emitRO("MUL", AC, AC1, AC, "op *"); break;
                        case OVER: emitRO("DIV", AC, AC1, AC, "op /"); break;
                        case LT: emitRO("SUB", AC, AC1, AC, "op <"); emitRM("JLT",AC,2,PC,"br"); emitRM("LDC",AC,0,0,"false"); emitRM("LDA",PC,1,PC,"skip"); emitRM("LDC",AC,1,0,"true"); break;
                        case GT: emitRO("SUB", AC, AC1, AC, "op >"); emitRM("JGT",AC,2,PC,"br"); emitRM("LDC",AC,0,0,"false"); emitRM("LDA",PC,1,PC,"skip"); emitRM("LDC",AC,1,0,"true"); break;
                        case EQ: emitRO("SUB", AC, AC1, AC, "op =="); emitRM("JEQ",AC,2,PC,"br"); emitRM("LDC",AC,0,0,"false"); emitRM("LDA",PC,1,PC,"skip"); emitRM("LDC",AC,1,0,"true"); break;
                        default: emitRO("SUB", AC, AC1, AC, "default"); break;
                    }
                    break;
                default: break;
            }
            break;
        default: break;
    }
    cGen(t->sibling);
}
void performBackpatch() {
    CallList c = callList;
    while (c != NULL) {
        int addr = getFunctionAddr(c->name);
        if (addr != -1) {
            int savedLoc = emitLoc;
            emitBackup(c->loc);
            fprintf(code, "%3d:  %5s  %d,%d(%d)    %s\n", c->loc, "LDC", PC, addr, 0, "jump func");
            emitBackup(savedLoc);
        }
        c = c->next;
    }
}
void codeGen(TreeNode *syntaxTree, char *codefile) {
    fprintf(code, "* C-Minus TM Code\n");
    emitRM("LDC", MP, 29999, 0, "init SP"); 
    emitRM("LDC", GP, 0, 0, "init GP");
    emitRM("ST",  0, 0, 0, "clear 0");
    emitRM("LDC", AC, emitLoc + 2, 0, "ret addr");
    int mainJumpLoc = emitSkip(1);
    emitRO("HALT", 0, 0, 0, "END");
    cGen(syntaxTree);
    int mainAddr = getFunctionAddr("main");
    int saved = emitLoc;
    emitBackup(mainJumpLoc);
    fprintf(code, "%3d:  %5s  %d,%d(%d)    %s\n", mainJumpLoc, "LDC", PC, mainAddr, 0, "jump main");
    emitBackup(saved);
    performBackpatch();
    printf("Código Stack-Machine gerado em %s\n", codefile);
}
