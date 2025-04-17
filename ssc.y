%{
    #include <stdio.h>
    #include <stdlib.h>
    extern int yyparse();
    extern int yylex();
    extern FILE *yyin;
    #define YYDEBUG 1

    //#define DEBUGBISON
    // This code is for producing debug output.
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
    ASTNode* ast_node;
    std::vector<ASTNode*>* stmt_list;
    std::vector<std::string>* param_list;  // Change to std::vector<std::string>
}


%token tok_printd
%token tok_prints
%token tok_if
%token tok_else
%token tok_for
%token tok_return
%token tok_def
%token <identifier> tok_identifier
%token <double_literal> tok_double_literal
%token <string_literal> tok_string_literal

%type <ast_node> term expression comparison statement assignment printd prints if_stmt for_stmt for_assign func_stmt func_call_stmt
%type <stmt_list> statements argument_list
%type <param_list> parameter_list

%left '+' '-' 
%left '*' '/'
%left '(' ')'
%left '<' '>' tok_LE tok_GE tok_EQ tok_NEQ tok_AddOne tok_SubOne
%start root

%%

/* Root production: a list of statements followed by a call to addReturnInstr() */
root:
      statements { 
          /* Traverse the AST and call codegen() on each node */
          for (ASTNode* node : *$1) {
            node->codegen();
            delete node;
          }

          delete $1;

          addReturnInstr(); 
          if (verifyModule(*module, &errs())) {
              fprintf(stderr, "Module verification failed!\n");
              exit(EXIT_FAILURE);  // Or handle error gracefully
          }

      }
    ;

/* Build a list of statements. */
statements:statement { $$ = new std::vector<ASTNode*>(); if ($1) $$->push_back($1); }
    | statements statement { if (!$1) $$ = new std::vector<ASTNode*>(); $$->push_back($2); }

/* A statement can be one of several constructs. */
statement:
      assignment { $$ = $1; }
    | expression ';' { $$ = $1; }
    | printd     { $$ = $1; }
    | prints     { $$ = $1; }
    | if_stmt    { $$ = $1; }
    | for_stmt   { $$ = $1; }
    | func_stmt  { $$ = $1; }
    | func_call_stmt { $$ = $1; }
    ;

/* Print a string (for print statements). */
prints:
      tok_prints '(' tok_string_literal ')' ';' { debugBison(5); $$ = new PrintStrAST(std::string($3)); }
    ;

/* Print a double value. */
printd:
      tok_printd '(' term ')' ';' { debugBison(6); $$ = new PrintDoubleAST($3); }
    ;

/* Terminals: identifiers and numeric literals.
   Create an IdentifierAST or NumberAST respectively. */
term:
      tok_identifier { debugBison(7);  $$ = new IdentifierAST(std::string($1)); free($1); }
    | tok_double_literal { debugBison(8);  $$ = new NumberAST($1); }
    ;

/* Assignment: create an AST node representing an assignment. */
assignment:
      tok_identifier '=' expression ';'  {  debugBison(9); $$ = new AssignmentAST(std::string($1), $3); free($1); }
    ;

/* Expression: binary operations and grouping. */
expression:
      term { debugBison(10); $$ = $1; }
    | expression tok_AddOne { debugBison(11); $$ = new BinaryOpAST($1, nullptr, '+'); }
    | expression tok_SubOne { debugBison(11); $$ = new BinaryOpAST($1, nullptr, '-'); }
    | expression '+' expression { debugBison(12); $$ = new BinaryOpAST($1, $3, '+'); }
    | expression '-' expression { debugBison(13); $$ = new BinaryOpAST($1, $3, '-'); }
    | expression '/' expression { debugBison(14); $$ = new BinaryOpAST($1, $3, '/'); }
    | expression '*' expression { debugBison(15); $$ = new BinaryOpAST($1, $3, '*'); }
    | '(' expression ')' { debugBison(16); $$ = $2; }
    ;

/* Comparison operations. */
comparison:
      expression { debugBison(17); $$ = new ComparisonAST(6, $1,nullptr); }
    | '!' expression { debugBison(18); $$ = new ComparisonAST(3, $2,nullptr); }
    | expression '>' expression { debugBison(19); $$ = new ComparisonAST(1, $1, $3); }
    | expression '<' expression { debugBison(20); $$ = new ComparisonAST(2, $1, $3); }
    | expression tok_EQ expression { debugBison(21); $$ = new ComparisonAST(3, $1, $3); }
    | expression tok_LE expression { debugBison(22); $$ = new ComparisonAST(4, $1, $3); }
    | expression tok_GE expression { debugBison(23); $$ = new ComparisonAST(5, $1, $3); }
    | expression tok_NEQ expression { debugBison(24); $$ = new ComparisonAST(6, $1, $3); }
    ;

/* If-statement: create an AST node for the if-statement.
   (The else branch is omitted in this example.) */
if_stmt:
    tok_if '(' comparison ')' '{' statements '}'  { debugBison(25); $$ = new IfAST($3, *$6, {}); }
    | tok_if '(' comparison ')' '{' statements '}' tok_else '{' statements '}'  { debugBison(26); $$ = new IfAST($3, *$6, *$10); }
    ;

for_assign:
      tok_identifier '=' expression { 
          debugBison(27); 
          $$ = new AssignmentAST(std::string($1), $3); 
          free($1);
      }
    ;

for_stmt: tok_for '(' for_assign ';' comparison ';' expression ')' '{' statements '}' { 
    debugBison(28); 
    $$ = new ForAST($3, $5, $7, *$10); 
}
| tok_for '(' comparison ';' expression ')' '{' statements '}' {
    $$ = new ForAST(nullptr, $3, $5, *$8);
}
;

parameter_list:
    tok_identifier {
        $$ = new std::vector<std::string>();
        $$->push_back(std::string($1));
    }
    | parameter_list ',' tok_identifier {
        $$ = $1;
        $$->push_back(std::string($3));
    }
    | /* empty */ {
        $$ = new std::vector<std::string>();
    }
;

func_stmt: tok_def tok_identifier '(' parameter_list ')' '{' statements '}' {
    debugBison(29);
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

func_call_stmt: tok_identifier '(' argument_list ')' ';' {
        $$ = new FuncCallAST(std::string($1), *$3); 
    }
    ;

%%


int main(int argc, char** argv) {
    if (argc > 1) {
        FILE *fp = fopen(argv[1], "r");
        yyin = fp; // read from file if name provided.
    } 
    if (yyin == NULL) { 
        yyin = stdin; // otherwise read from terminal.
    }
    
    // Initialize LLVM.
    initLLVM();
    
    // Parse input and build the AST.
    int parserResult = yyparse();
    
    // Print the generated LLVM IR.
    printLLVMIR();
    
    return EXIT_SUCCESS;
}
