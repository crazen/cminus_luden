
%code requires {
    #include "globals.h"
    #include "ast.h"
    #define YYSTYPE TreeNode *
}

%{
#define _POSIX_C_SOURCE 200809L
#include "globals.h"
#include "util.h"
#include "scan.h"
#include "symtab.h"
#include <string.h>

static ExpType currentType;
static TreeNode *savedTree; 

/* Trazemos o tokenString do scanner para saber qual texto causou erro */
extern char tokenString[256];

int yylex(void);
void yyerror(char *message);
%}

%define parse.error verbose

%token IF ELSE INT RETURN VOID WHILE 
%token ID NUM 
%token ASSIGN EQ NE LT LE GT GE PLUS MINUS TIMES OVER
%token LPAREN RPAREN LBRACK RBRACK LBRACE RBRACE SEMI COMMA
%token ERROR

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

programa
    : declaration_list
      { savedTree = $1; }
    ;

declaration_list
    : declaration_list declaration
      { 
        YYSTYPE t = $1;
        if (t != NULL) {
            while (t->sibling != NULL) t = t->sibling;
            t->sibling = $2;
            $$ = $1;
        } else $$ = $2;
      }
    | declaration { $$ = $1; }
    ;

declaration
    : type_specifier ID 
      {
        currentType = ($1->attr.name[0]=='i') ? Integer : Void;
      }
      declaration_suffix
      { 
        $$ = $4;
        if ($$ != NULL) {
            $$->attr.name = $2->attr.name;
            $$->type = ($1->attr.name[0]=='i') ? Integer : Void;
            $$->lineno = $2->lineno;
        }
      }
    | error SEMI 
      { 
        yyerrok; 
        $$ = NULL; 
      }
    ;

type_specifier
    : INT  { $$ = newExpNode(TypeK); $$->attr.name = "int"; }
    | VOID { $$ = newExpNode(TypeK); $$->attr.name = "void"; }
    ;

declaration_suffix
    : SEMI 
      { $$ = newStmtNode(VarDeclK); }
    | LBRACK NUM RBRACK SEMI
      {
        $$ = newStmtNode(VarDeclK);
        $$->child[0] = $2; 
      }
    | COMMA var_decl_list SEMI
      {
        $$ = newStmtNode(VarDeclK);
        $$->sibling = $2;
      }
    | LPAREN params RPAREN compound_stmt
      {
        $$ = newStmtNode(FunDeclK);
        $$->child[0] = NULL;
        $$->child[1] = $2;
        $$->child[2] = $4;
      }
    ;

var_decl_list
    : var_decl_id COMMA var_decl_list
      { $1->sibling = $3; $$ = $1; }
    | var_decl_id
      { $$ = $1; }
    ;

var_decl_id
    : ID
      {
        $$ = newStmtNode(VarDeclK);
        $$->attr.name = $1->attr.name;
        $$->type = currentType;
      }
    | ID LBRACK NUM RBRACK
      {
        $$ = newStmtNode(VarDeclK);
        $$->attr.name = $1->attr.name;
        $$->type = currentType;
        $$->child[0] = $3;
      }
    ;

params
    : param_list { $$ = $1; }
    | VOID { $$ = NULL; }
    ;

param_list
    : param_list COMMA param
      {
        YYSTYPE t = $1;
        while (t->sibling) t = t->sibling;
        t->sibling = $3;
        $$ = $1;
      }
    | param { $$ = $1; }
    ;

param
    : type_specifier ID
      {
        $$ = newStmtNode(ParamK);
        $$->attr.name = $2->attr.name;
        $$->type = ($1->attr.name[0]=='i') ? Integer : Void;
      }
    | type_specifier ID LBRACK RBRACK
      {
        $$ = newStmtNode(ParamK);
        $$->attr.name = $2->attr.name;
        $$->type = ($1->attr.name[0]=='i') ? Integer : Void;
      }
    ;

compound_stmt
    : LBRACE { scope_push("local"); } local_declarations statement_list RBRACE 
      { 
        scope_pop();
        $$ = newStmtNode(CompoundK);
        $$->child[0] = $3;
        $$->child[1] = $4;
      }
    | LBRACE error RBRACE 
      { 
        yyerrok;
        $$ = NULL; 
      }
    ;

local_declarations
    : local_declarations declaration 
      {
        YYSTYPE t = $1;
        if (t != NULL) { while (t->sibling) t = t->sibling; t->sibling = $2; $$ = $1; }
        else $$ = $2;
      }
    | /* vazio */ { $$ = NULL; }
    ;

statement_list
    : statement_list statement
      {
        YYSTYPE t = $1;
        if (t != NULL) { while (t->sibling) t = t->sibling; t->sibling = $2; $$ = $1; }
        else $$ = $2;
      }
    | /* vazio */ { $$ = NULL; }
    ;

statement
    : expression_stmt { $$ = $1; }
    | compound_stmt { $$ = $1; }
    | selection_stmt { $$ = $1; }
    | iteration_stmt { $$ = $1; }
    | return_stmt { $$ = $1; }
    | error SEMI 
      { 
         yyerrok;
         $$ = NULL; 
      }
    ;

expression_stmt
    : expression SEMI { $$ = $1; }
    | SEMI { $$ = NULL; }
    | expression error 
      { 
         /* Este caso continua valido para erros genéricos */
         $$ = NULL;
      }
    ;

selection_stmt
    : IF LPAREN expression RPAREN statement %prec LOWER_THAN_ELSE
      { $$ = newStmtNode(IfK); $$->child[0] = $3; $$->child[1] = $5; }
    | IF LPAREN expression RPAREN statement ELSE statement
      { $$ = newStmtNode(IfK); $$->child[0] = $3; $$->child[1] = $5; $$->child[2] = $7; }
    | IF LPAREN error RPAREN statement 
      { 
        yyerrok; 
        $$ = NULL; 
      }
    ;

iteration_stmt
    : WHILE LPAREN expression RPAREN statement
      { $$ = newStmtNode(WhileK); $$->child[0] = $3; $$->child[1] = $5; }
    ;

return_stmt
    : RETURN SEMI { $$ = newStmtNode(ReturnK); }
    | RETURN expression SEMI { $$ = newStmtNode(ReturnK); $$->child[0] = $2; }
    ;

expression
    : var ASSIGN expression { $$ = newStmtNode(AssignK); $$->child[0] = $1; $$->child[1] = $3; }
    | simple_expression { $$ = $1; }
    ;

var
    : ID { $$ = $1; }
    | ID LBRACK expression RBRACK
      { $$ = $1; $$->child[0] = $3; }
    ;

simple_expression
    : additive_expression relop additive_expression
      { $$ = $2; $$->child[0] = $1; $$->child[1] = $3; }
    | additive_expression { $$ = $1; }
    ;

relop
    : LE { $$ = newExpNode(OpK); $$->attr.op = LE; }
    | LT { $$ = newExpNode(OpK); $$->attr.op = LT; }
    | GT { $$ = newExpNode(OpK); $$->attr.op = GT; }
    | GE { $$ = newExpNode(OpK); $$->attr.op = GE; }
    | EQ { $$ = newExpNode(OpK); $$->attr.op = EQ; }
    | NE { $$ = newExpNode(OpK); $$->attr.op = NE; }
    ;

additive_expression
    : additive_expression addop term { $$ = $2; $$->child[0] = $1; $$->child[1] = $3; }
    | term { $$ = $1; }
    ;

addop
    : PLUS { $$ = newExpNode(OpK); $$->attr.op = PLUS; }
    | MINUS { $$ = newExpNode(OpK); $$->attr.op = MINUS; }
    ;

term
    : term mulop factor { $$ = $2; $$->child[0] = $1; $$->child[1] = $3; }
    | factor { $$ = $1; }
    ;

mulop
    : TIMES { $$ = newExpNode(OpK); $$->attr.op = TIMES; }
    | OVER { $$ = newExpNode(OpK); $$->attr.op = OVER; }
    ;

factor
    : LPAREN expression RPAREN { $$ = $2; }
    | var { $$ = $1; }
    | call { $$ = $1; }
    | NUM { $$ = $1; }
    ;

call
    : ID LPAREN args RPAREN
      { $$ = newStmtNode(CallK); $$->attr.name = $1->attr.name; $$->child[0] = $3; }
    ;

args
    : arg_list { $$ = $1; }
    | /* vazio */ { $$ = NULL; }
    ;

arg_list
    : arg_list COMMA expression
      { YYSTYPE t = $1; while (t->sibling) t = t->sibling; t->sibling = $3; $$ = $1; }
    | expression { $$ = $1; }
    ;

%%

void yyerror(char *message) {
    /* HEURÍSTICA DE ERRO INTELIGENTE 
       Interceptamos a mensagem genérica do Bison e a tornamos específica.
    */
    char finalMsg[256];
    
    // Caso par1.c: "unexpected ID, expecting ... SEMI"
    if (strstr(message, "unexpected ID") && strstr(message, "SEMI")) {
        sprintf(finalMsg, "Faltou ponto e virgula ';' antes do identificador '%s'", tokenString);
    }
    // Caso par2.c: "unexpected ID, expecting LPAREN"
    else if (strstr(message, "unexpected ID") && strstr(message, "LPAREN")) {
         // Se for um IF ou WHILE, é parenteses
         sprintf(finalMsg, "Faltou abrir parenteses '(' antes de '%s'", tokenString);
    }
    // Caso par4.c: "unexpected SEMI" em expressão
    else if (strstr(message, "unexpected SEMI")) {
        sprintf(finalMsg, "Expressao incompleta ou invalida antes do ';'");
    }
    // Erro de caractere inválido
    else if (strstr(message, "unexpected ERROR")) {
        sprintf(finalMsg, "Caractere invalido ou nao reconhecido na linguagem");
    }
    // Caso genérico
    else {
        strcpy(finalMsg, message);
    }

    fprintf(listing, ">>> ERRO SINTATICO (Linha %d): %s\n", lineno, finalMsg);
    Error = TRUE;
}

TreeNode *parse(void) {
    yyparse();
    return savedTree;
}
