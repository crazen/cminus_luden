/* util.c */
#include "globals.h"
#include "util.h"
#include "parse.h" 

TreeNode * newStmtNode(StmtKind kind) {
    TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
    int i;
    if (t==NULL) fprintf(listing,"Erro memoria\n");
    else {
        for (i=0;i<MAXCHILDREN;i++) t->child[i] = NULL;
        t->sibling = NULL;
        t->nodekind = StmtK;
        t->kind.stmt = kind;
        t->lineno = lineno;
        t->attr.name = NULL;
        t->memloc = 0;
        t->scope = 0; /* Padrão Global */
    }
    return t;
}

TreeNode * newExpNode(ExpKind kind) {
    TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
    int i;
    if (t==NULL) fprintf(listing,"Erro memoria\n");
    else {
        for (i=0;i<MAXCHILDREN;i++) t->child[i] = NULL;
        t->sibling = NULL;
        t->nodekind = ExpK;
        t->kind.exp = kind;
        t->lineno = lineno;
        t->type = Void;
        t->attr.name = NULL;
        t->memloc = 0;
        t->scope = 0; /* Padrão Global */
    }
    return t;
}

char * copyString(char * s) {
    int n;
    char * t;
    if (s==NULL) return NULL;
    n = strlen(s)+1;
    t = malloc(n);
    if (t==NULL) fprintf(listing,"Erro memoria\n");
    else strcpy(t,s);
    return t;
}

void printToken(TokenType token, const char* tokenString) {
    switch (token) {
        case IF: case ELSE: case INT: case RETURN: case VOID:
        case WHILE: case INPUT: case OUTPUT:
            fprintf(listing,"reserved: %s\n",tokenString); break;
        case ASSIGN: fprintf(listing,"=\n"); break;
        case EQ: fprintf(listing,"==\n"); break;
        case NE: fprintf(listing,"!=\n"); break;
        case LT: fprintf(listing,"<\n"); break;
        case LE: fprintf(listing,"<=\n"); break;
        case GT: fprintf(listing,">\n"); break;
        case GE: fprintf(listing,">=\n"); break;
        case PLUS: fprintf(listing,"+\n"); break;
        case MINUS: fprintf(listing,"-\n"); break;
        case TIMES: fprintf(listing,"*\n"); break;
        case OVER: fprintf(listing,"/\n"); break;
        case LPAREN: fprintf(listing,"(\n"); break;
        case RPAREN: fprintf(listing,")\n"); break;
        case LBRACK: fprintf(listing,"[\n"); break;
        case RBRACK: fprintf(listing,"]\n"); break;
        case LBRACE: fprintf(listing,"{\n"); break;
        case RBRACE: fprintf(listing,"}\n"); break;
        case SEMI: fprintf(listing,";\n"); break;
        case COMMA: fprintf(listing,",\n"); break;
        case NUM: fprintf(listing,"NUM, val= %s\n",tokenString); break;
        case ID: fprintf(listing,"ID, name= %s\n",tokenString); break;
        case ERROR: fprintf(listing,"ERROR: %s\n",tokenString); break;
        default: fprintf(listing,"Unknown token: %d\n",token);
    }
}

static int indentno = 0;
#define INDENT indentno+=2
#define UNINDENT indentno-=2

static void printSpaces(void) {
    int i;
    for (i=0; i<indentno; i++) fprintf(listing," ");
}

void printTree(TreeNode *tree, int level, int siblingNum) {
    int i;
    (void)level; (void)siblingNum;
    INDENT;
    while (tree != NULL) {
        printSpaces();
        if (tree->nodekind == StmtK) {
            switch (tree->kind.stmt) {
                case IfK:      fprintf(listing,"If\n"); break;
                case WhileK:   fprintf(listing,"While\n"); break;
                case AssignK:  fprintf(listing,"Assign\n"); break;
                case ReturnK:  fprintf(listing,"Return\n"); break;
                case CallK:    fprintf(listing,"Call: %s\n",tree->attr.name); break;
                case VarDeclK: fprintf(listing,"Var Decl: %s\n",tree->attr.name); break;
                case FunDeclK: fprintf(listing,"Function Decl: %s\n",tree->attr.name); break;
                case ParamK:   fprintf(listing,"Param: %s\n",tree->attr.name); break;
                case CompoundK:fprintf(listing,"Compound Statement\n"); break;
                default:       fprintf(listing,"Unknown StmtNode\n"); break;
            }
        }
        else if (tree->nodekind == ExpK) {
            switch (tree->kind.exp) {
                case OpK:
                    fprintf(listing,"Op: ");
                    printToken(tree->attr.op,"");
                    break;
                case ConstK:
                    fprintf(listing,"Const: %d\n",tree->attr.val);
                    break;
                case IdK:
                    fprintf(listing,"Id: %s\n",tree->attr.name);
                    break;
                case TypeK:
                    fprintf(listing,"Type: %s\n", tree->attr.name);
                    break;
                default:
                    fprintf(listing,"Unknown ExpNode\n");
                    break;
            }
        }
        else fprintf(listing,"Unknown node kind\n");

        for (i=0; i<MAXCHILDREN; i++)
            if (tree->child[i]) printTree(tree->child[i], level+1, i);
        tree = tree->sibling;
    }
    UNINDENT;
}