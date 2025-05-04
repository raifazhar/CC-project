%{
    #include <stdio.h>
    #include <stdlib.h>
    extern int yyparse();
    extern int yylex();
    extern int yylineno;  // Add this line to declare yylineno
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
    class TypeAST;  // Forward declaration
}

%union {
    char *identifier;
    int integer_literal;
    double real_literal;
    char *string_literal;
    char char_literal;
    bool boolean_literal;
    char* date_literal;
    ASTNode* ast_node;
    TypeAST* type_node;
    ComparisonAST* comparison_ast;
    ArrayAssignmentAST* array_assignment_ast;
    AssignmentAST* assignment_ast;
    BinaryOpAST* binary_ast;
    RealLiteralAST* real_ast;
InputAST* input_ast;
    std::vector<ASTNode*>* stmt_list;
std::vector<OutputAST*>* output_list;
    std::vector<ParameterAST*>* param_list;
    StatementBlockAST* statement_block_ast;
}

%token <identifier> tok_Identifier
%token <integer_literal> tok_Integer_Literal
%token <real_literal> tok_Real_Literal
%token <string_literal> tok_String_Literal
%token <char_literal> tok_Char_Literal          
%token <boolean_literal> tok_Bool_Literal  
%token <date_literal> tok_Date_Literal

%token tok_Declare
%token tok_Array
%token tok_Of
%token tok_Output tok_Input
%token tok_If tok_Else tok_End_If
%token tok_While tok_End_While
%token tok_Repeat tok_Until
%token tok_For tok_Next tok_To tok_Step 
%token tok_Procedure tok_End_Procedure
%token tok_Function tok_End_Function tok_Returns tok_Return
%token tok_Call
%token tok_Integer tok_Real tok_Boolean tok_Char tok_String tok_Date

%token tok_Indent tok_Dedent tok_Newline
%token tok_AddOne "++"
%token tok_SubOne "--"
%token tok_EQ "=="
%token tok_NEQ "!="
%token tok_LE "<="
%token tok_GE ">="
%token tok_And "AND"
%token tok_Or "OR"
%token tok_Not "NOT"

%left '+' '-'
%left '*' '/'
%left tok_Or
%left tok_And
%right tok_Not
%nonassoc '<' '>' tok_LE tok_GE tok_EQ tok_NEQ
%left tok_AddOne tok_SubOne
%right UMINUS

%type <integer_literal> opt_step integer_expr
%type <ast_node> statement expression term
%type <ast_node> if_stmt for_stmt while_stmt repeat_stmt output
%type <ast_node> procedure_stmt function_stmt func_call_stmt return_stmt declaration
%type <comparison_ast> comparison
%type <assignment_ast> assignment
%type <array_assignment_ast> array_assignment
%type <input_ast> input
%type <type_node> type
%type <stmt_list> statements statement_line argument_list
%type <param_list> parameter_list
%type <statement_block_ast> statement_block

%start root

%%

root:
    statements {
        fprintf(stderr, "Processing %zu statements\n", $1->size());

        globalSymbolTable->enterScope();

        bool hasError = false;

        for (ASTNode* node : *$1) {
            fprintf(stderr, "Codegen for node type: %s\n", typeid(*node).name());
            Value* result = node->codegen();

            delete node;
        }

        globalSymbolTable->exitScope();
        delete $1;

        addReturnInstr();
        fprintf(stderr, "Added return instruction\n");

        fprintf(stderr, "Module verification passed\n");
        llvm::errs().flush();
    }
;

statements:
    statement_line { 
        fprintf(stderr, "DEBUG: Creating new statements list with single statement_line\n"); 
        $$ = $1; 
    }
    | statements statement_line {
        fprintf(stderr, "DEBUG: Appending statement_line to existing statements list\n");
        fprintf(stderr, "DEBUG: Current statements size: %zu\n", $1->size());
        $$ = $1;
        $$->insert($$->end(), $2->begin(), $2->end());
        fprintf(stderr, "DEBUG: New statements size after append: %zu\n", $$->size());
        delete $2;
    }
;

statement_line:
    statement opt_newline {
        $$ = new std::vector<ASTNode*>();
        if ($1) $$->push_back($1);
    }
;

opt_newline:
    tok_Newline
  | /* empty */
;


statement:
      assignment  { fprintf(stderr, "DEBUG: Processing assignment statement\n"); $$ = $1; }
    | array_assignment { fprintf(stderr, "DEBUG: Processing array assignment statement\n"); $$ = $1; }
    | expression  { fprintf(stderr, "DEBUG: Processing expression statement\n"); $$ = $1; }
    | output {fprintf(stderr, "DEBUG: Processing output statement\n"); $$ = $1; }
    | input  {fprintf(stderr, "DEBUG: Processing input statement\n"); $$ = $1; }
    | if_stmt { fprintf(stderr, "DEBUG: Processing if statement\n"); $$ = $1; }
    | for_stmt { fprintf(stderr, "DEBUG: Processing for statement\n"); $$ = $1; }
    | while_stmt { fprintf(stderr, "DEBUG: Processing while statement\n"); $$ = $1; }
    | repeat_stmt { fprintf(stderr, "DEBUG: Processing repeat statement\n"); $$ = $1; }
    | procedure_stmt { fprintf(stderr, "DEBUG: Processing procedure statement\n"); $$ = $1; }
    | function_stmt { fprintf(stderr, "DEBUG: Processing function statement\n"); $$ = $1; }
    | declaration { fprintf(stderr, "DEBUG: Processing declaration statement\n"); $$ = $1; }
    | return_stmt { fprintf(stderr, "DEBUG: Processing return statement\n"); $$=$1; }
;


type:
      tok_Integer { $$ = new TypeAST("INTEGER"); }
    | tok_Real { $$ = new TypeAST("REAL"); }
    | tok_Boolean { $$ = new TypeAST("BOOLEAN"); }
    | tok_String { $$ = new TypeAST("STRING"); }
    | tok_Char { $$ = new TypeAST("CHAR"); }
    | tok_Date { $$ = new TypeAST("date"); }
;

declaration:
    tok_Declare tok_Identifier ':' type{

        $$ = new DeclarationAST(new IdentifierAST(std::string($2)), $4);
        free($2);
    }
    | tok_Declare tok_Identifier ':' tok_Array'[' tok_Integer_Literal ':' tok_Integer_Literal ']' tok_Of type {
        $$ = new ArrayAST(new IdentifierAST(std::string($2)), $11,$6, $8);
        free($2);
    }
    | tok_Declare tok_Identifier ':' tok_Array'[' ':' tok_Integer_Literal ']' tok_Of type {
        $$ = new ArrayAST(new IdentifierAST(std::string($2)), $10,  0,$7);
        free($2);
    }
;

assignment:
    tok_Identifier '=' expression {
        $$ = new AssignmentAST(new IdentifierAST(std::string($1)), $3); free($1);
    }
;

array_assignment:
    tok_Identifier '[' expression ']' '=' expression {
        $$ = new ArrayAssignmentAST(new IdentifierAST(std::string($1)), $6, $3); free($1);
    }
;

term:
      tok_Identifier { $$ = new IdentifierAST(std::string($1)); free($1); }
    | tok_Integer_Literal { $$ = new IntegerLiteralAST($1); }
    | tok_Real_Literal { $$ = new RealLiteralAST($1); }
    | tok_String_Literal { $$ = new StringLiteralAST(std::string($1)); free($1); }
    | tok_Bool_Literal { $$ = new BooleanLiteralAST($1); }
    | tok_Char_Literal { $$ = new CharLiteralAST($1); }
    | tok_Date_Literal { $$ = new DateLiteralAST(std::string($1)); free($1); }
;


output:
    tok_Output expression {
        // Single expression: create OutputAST
        auto outputs = std::vector<ASTNode*>();
        outputs.push_back($2);
        $$ = new OutputAST(outputs);
    }
    | output ',' expression {
        // Add expression to existing OutputAST
        auto outputAST = dynamic_cast<OutputAST*>($1);
        if (outputAST) {
            outputAST->expressions.push_back($3);
            $$ = outputAST;
        } else {
            // Should not happen, but safety fallback
            auto outputs = std::vector<ASTNode*>();
            outputs.push_back($3);
            $$ = new OutputAST(outputs);
        }
    }
;

input:
    tok_Input tok_Identifier { errs()<<"Input int working\n"; $$= new InputAST(new IdentifierAST($2));}
;


expression:
      term { $$ = $1; }
    | expression tok_AddOne { $$ = new BinaryOpAST($1, nullptr, "++"); }
    | expression tok_SubOne { $$ = new BinaryOpAST($1, nullptr, "--"); }
    | expression '+' expression { $$ = new BinaryOpAST($1, $3, "+"); }
    | expression '-' expression { $$ = new BinaryOpAST($1, $3, "-"); }
    | expression '*' expression { $$ = new BinaryOpAST($1, $3, "*"); }
    | expression '/' expression { $$ = new BinaryOpAST($1, $3, "/"); }
    | '-' expression      %prec UMINUS {$$= new UnaryOpAST($2,"-");}
    | func_call_stmt {$$=$1;}
;



comparison:
      expression '>' expression { $$ = new ComparisonAST( $1, $3, ">"); }
    | expression '<' expression { $$ = new ComparisonAST( $1, $3,"<"); }
    | expression tok_EQ expression { $$ = new ComparisonAST( $1, $3, "=="); }
    | expression tok_LE expression { $$ = new ComparisonAST($1, $3,"<="); }
    | expression tok_GE expression { $$ = new ComparisonAST($1, $3,">="); }
    | expression tok_NEQ expression { $$ = new ComparisonAST( $1, $3,"!="); }
    | comparison tok_And comparison { $$ = new LogicalOpAST($1, $3, "AND"); }
    | comparison tok_Or comparison { $$ = new LogicalOpAST($1, $3, "OR"); }
    |  tok_Not comparison { $$ = new LogicalOpAST(nullptr, $2, "NOT"); }
;

statement_block: 
    tok_Newline tok_Indent statements tok_Dedent { 
        fprintf(stderr, "DEBUG: Creating statement block with dedent\n"); 
        if ($3 == nullptr) {
            fprintf(stderr, "DEBUG: Warning - null statements in block\n");
            $$ = new StatementBlockAST(std::vector<ASTNode*>());
        } else {
            fprintf(stderr, "DEBUG: Block contains %zu statements\n", $3->size());
            $$ = new StatementBlockAST(*$3); 
        }
        delete $3;
    }
;

if_stmt:
    tok_If comparison statement_block tok_End_If {
        $$ = new IfAST($2, $3, {});
    }
    | tok_If comparison statement_block tok_Else statement_block tok_End_If {
        $$ = new IfAST($2, $3, $5);
    }
;

for_stmt:
    tok_For assignment tok_To expression opt_step statement_block tok_Next tok_Identifier {
        if (strcmp($8, $2->identifier->name.c_str()) != 0) {
            yyerror("Loop variable mismatch");
            YYERROR;
        }

        auto loopVar = $2->identifier;
        auto startLit = dynamic_cast<IntegerLiteralAST*>($2->expression);
        auto endLit = dynamic_cast<IntegerLiteralAST*>($4);
        auto cond = new ComparisonAST(loopVar, $4,(startLit && endLit && startLit->value > endLit->value) ? ">=" : "<=");

        // Determine step size
        auto step = $5 ? new IntegerLiteralAST($5) : new IntegerLiteralAST(1);

        auto binaryOp = new BinaryOpAST(loopVar, step, "+");
        auto incrementAssign = new AssignmentAST(loopVar, binaryOp, nullptr);

        $$ = new ForAST($2, cond, incrementAssign, $6);
        free($8);
    }
;

// handle optional "STEP"
opt_step:
    /* empty */           { $$ = 1; }
  | tok_Step integer_expr { $$ = $2; }
;

integer_expr:
    tok_Integer_Literal   { $$ = $1; }
  | '-' tok_Integer_Literal { $$ = -$2; }
;


while_stmt:
    tok_While comparison statement_block tok_End_While {
        $$ = new WhileAST($2, $3->statements);
    }
;

repeat_stmt:
    tok_Repeat statement_block tok_Until comparison {
        $$ = new RepeatAST($4, $2->statements);
    }
;

parameter_list:
       tok_Identifier ':' type {
          $$ = new std::vector<ParameterAST *>();
          $$->push_back(new ParameterAST($3, std::string($1)));
      }
    | parameter_list ',' tok_Identifier ':' type {
          $$ = $1;
          $$->push_back(new ParameterAST($5, std::string($3)));
      }
    | /* empty */ {
          $$ = new std::vector<ParameterAST *>();
      }
;

procedure_stmt:
    tok_Procedure tok_Identifier '(' parameter_list ')' statement_block tok_End_Procedure {
        $$ = new ProcedureAST(new IdentifierAST($2), *$4, $6);
        free($2);
    }
;

function_stmt:
   tok_Function tok_Identifier '(' parameter_list ')' tok_Returns type statement_block tok_End_Function 
    { 
        $$ = new FuncAST(new IdentifierAST($2), *$4, $8, $7);
        free($2);
    }
;

return_stmt:
    tok_Return expression {
        $$ = new ReturnAST($2);
    };

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
    tok_Call tok_Identifier '(' argument_list ')' {
        $$ = new FuncCallAST(std::string($2), *$4); 
    }
;

%%

int main(int argc, char** argv) {
        if (argc > 1) {
        FILE *fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "Error opening file: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
    yyin = fp;
        fprintf(stderr, "Opened input file: %s\n", argv[1]);
    } 
    if (yyin == NULL) {
        yyin = stdin;
        fprintf(stderr, "Using stdin as input\n");
    }

    initLLVM();
    fprintf(stderr, "LLVM initialized\n");
    
    int parserResult = yyparse();
    fprintf(stderr, "Parser result: %d\n", parserResult);

    printLLVMIR();
    fprintf(stderr, "LLVM IR printed\n");
    
    return EXIT_SUCCESS;
}
