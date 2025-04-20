%{
    #include <stdio.h>
    #include <stdlib.h>
    extern int yyparse();
    extern int yylex();
    extern FILE *yyin;
    #define YYDEBUG 1

    #ifdef DEBUGBISON
        #define debugBison(a) (printf("\n%d \n", a))
    #else
        #define debugBison(a)
    #endif
%}

%code requires {
    #include "AST.h"
    #include <vector>
}

%union {
    char *identifier;
    double double_literal;
    char *string_literal;
    char char_literal;
    bool boolean_literal;
    char* date_literal;
    ASTNode* ast_node;
    std::vector<ASTNode*>* stmt_list;
    std::vector<std::string>* param_list;
}

%token <identifier> tok_Identifier
%token <double_literal> tok_Double_Literal
%token <string_literal> tok_String_Literal
%token <char_literal> tok_Char_Literal          
%token <boolean_literal> tok_Bool_Literal  
%token <date_literal> tok_Date_Literal

%token tok_Printd tok_Prints tok_Declare tok_If tok_Else tok_For tok_Return tok_Returns tok_Function tok_Def

%token tok_Int tok_Real tok_Boolean tok_Char tok_String tok_Date

%token tok_And tok_Or
%token tok_AddOne "++"
%token tok_SubOne "--"
%token tok_EQ "=="
%token tok_NEQ "!="
%token tok_LE "<="
%token tok_GE ">="

%left '+' '-' '*' '/' '<' '>' tok_LE tok_GE tok_EQ tok_NEQ tok_AddOne tok_SubOne

%type <ast_node> term expression comparison statement assignment printd prints if_stmt for_stmt for_assign func_stmt func_call_stmt declaration type
%type <stmt_list> statements argument_list
%type <param_list> parameter_list

%start root

%%

root:
    statements {
        for (ASTNode* node : *$1) {
            node->codegen();
            delete node;
        }
        delete $1;
        addReturnInstr();

        if (verifyModule(*module, &errs())) {
            fprintf(stderr, "Module verification failed!\n");
            exit(EXIT_FAILURE);
        }
    }
;

statements:
      statement { $$ = new std::vector<ASTNode*>(); if ($1) $$->push_back($1); }
    | statements statement { if (!$1) $$ = new std::vector<ASTNode*>(); $$->push_back($2); }
;

statement:
      assignment { $$ = $1; }
    | expression ';' { $$ = $1; }
    | printd { $$ = $1; }
    | prints { $$ = $1; }
    | if_stmt { $$ = $1; }
    | for_stmt { $$ = $1; }
    | func_stmt { $$ = $1; }
    | func_call_stmt { $$ = $1; }
;

prints:
    tok_Prints '(' tok_String_Literal ')' ';' { $$ = new PrintStrAST(std::string($3)); }
;

printd:
    tok_Printd '(' term ')' ';' { $$ = new PrintDoubleAST($3); }
;

term:
      tok_Identifier { $$ = new IdentifierAST(std::string($1)); free($1); }
    | tok_Double_Literal { $$ = new NumberAST($1); }
    | tok_String_Literal { $$ = new StringAST(std::string($1)); }
    | tok_Bool_Literal { $$ = new BoolAST($1); }
;

type:
      tok_Int { $$ = new TypeAST("int"); }
    | tok_Real { $$ = new TypeAST("real"); }
    | tok_Boolean { $$ = new TypeAST("boolean"); }
    | tok_String { $$ = new TypeAST("string"); }
    | tok_Char { $$ = new TypeAST("char"); }
    | tok_Date { $$ = new TypeAST("date"); }
;

declaration:
    tok_Declare tok_Identifier ':' type
;

assignment:
    tok_Identifier '=' expression ';' {
        $$ = new AssignmentAST(std::string($1), $3); free($1);
    }
;

expression:
      term { $$ = $1; }
    | expression tok_AddOne { $$ = new BinaryOpAST($1, nullptr, '+'); }
    | expression tok_SubOne { $$ = new BinaryOpAST($1, nullptr, '-'); }
    | expression '+' expression { $$ = new BinaryOpAST($1, $3, '+'); }
    | expression '-' expression { $$ = new BinaryOpAST($1, $3, '-'); }
    | expression '/' expression { $$ = new BinaryOpAST($1, $3, '/'); }
    | expression '*' expression { $$ = new BinaryOpAST($1, $3, '*'); }
    | '(' expression ')' { $$ = $2; }
;

comparison:
      comparison tok_And comparison { $$ = new LogicalOpAST("and", $1, $3); }
    | comparison tok_Or comparison { $$ = new LogicalOpAST("or", $1, $3); }
    | expression { $$ = new ComparisonAST(6, $1, nullptr); }
    | '!' expression { $$ = new ComparisonAST(3, $2, nullptr); }
    | expression '>' expression { $$ = new ComparisonAST(1, $1, $3); }
    | expression '<' expression { $$ = new ComparisonAST(2, $1, $3); }
    | expression tok_EQ expression { $$ = new ComparisonAST(3, $1, $3); }
    | expression tok_LE expression { $$ = new ComparisonAST(4, $1, $3); }
    | expression tok_GE expression { $$ = new ComparisonAST(5, $1, $3); }
    | expression tok_NEQ expression { $$ = new ComparisonAST(6, $1, $3); }
;

if_stmt:
      tok_If '(' comparison ')' '{' statements '}' { $$ = new IfAST($3, *$6, {}); }
    | tok_If '(' comparison ')' '{' statements '}' tok_Else '{' statements '}' { $$ = new IfAST($3, *$6, *$10); }
    | tok_If '(' comparison ')' '{' statements '}' tok_Else tok_If '(' comparison ')' '{' statements '}' {}
    ;
;

for_assign:
    tok_Identifier '=' expression { $$ = new AssignmentAST(std::string($1), $3); free($1); }
;

for_stmt:
      tok_For '(' for_assign ';' comparison ';' expression ')' '{' statements '}' {
          $$ = new ForAST($3, $5, $7, *$10);
      }
    | tok_For '(' comparison ';' expression ')' '{' statements '}' {
          $$ = new ForAST(nullptr, $3, $5, *$8);
      }
;

parameter_list:
      tok_Identifier {
          $$ = new std::vector<std::string>();
          $$->push_back(std::string($1));
      }
    | parameter_list ',' tok_Identifier {
          $$ = $1;
          $$->push_back(std::string($3));
      }
    | /* empty */ {
          $$ = new std::vector<std::string>();
      }
;

func_stmt:
    tok_Def tok_Identifier '(' parameter_list ')' '{' statements '}' {
        $$ = new FuncAST(std::string($2), *$4, *$7);
    }
;

argument_list:
      expression {
          $$ = new std::vector<ASTNode*>();
          $$->push_back($1);
      }
    | argument_list ',' expression {
          $$ = $1;
          $$->push_back($3);
      }
    | /* empty */ {
          $$ = new std::vector<ASTNode*>();
      }
;

func_call_stmt:
    tok_Identifier '(' argument_list ')' ';' {
        $$ = new FuncCallAST(std::string($1), *$3); 
    }
;

%%

int main(int argc, char** argv) {
    if (argc > 1) {
        FILE *fp = fopen(argv[1], "r");
        yyin = fp;
    } 
    if (yyin == NULL) {
        yyin = stdin;
    }

    initLLVM();
    int parserResult = yyparse();
    printLLVMIR();

    return EXIT_SUCCESS;
}
