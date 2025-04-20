%{
#include <llvm/IR/Value.h>
#include "ssc.tab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <stack>

extern void yyerror(const char *msg);
extern int yylineno;

std::stack<int> indent_stack;
int current_indent = 0;

#define INDENT_WIDTH 4 // Define your indentation width here
%}

%option yylineno
%option noyywrap

%%

^[ \t]*\n {
    // Empty or whitespace-only line; ignore
    /* No action needed */
}

^[ \t]+ {
    int new_indent = 0;
    for (char *c = yytext; *c; ++c) {
        if (*c == ' ')
            new_indent += 1;
        else if (*c == '\t')
            new_indent += INDENT_WIDTH;
    }

    if (new_indent > current_indent) {
        indent_stack.push(current_indent);
        current_indent = new_indent;
        return tok_INDENT;
    } else if (new_indent < current_indent) {
        while (!indent_stack.empty() && indent_stack.top() > new_indent) {
            indent_stack.pop();
            return tok_DEDENT;
        }
        current_indent = new_indent;
    }
    // If indentation level is the same, do nothing
}

\n {
    return tok_NEWLINE;
}

"INTEGER"               { return tok_Integer; }
"REAL"                  { return tok_Real; }
"CHAR"                  { return tok_Char; }
"STRING"                { return tok_String; }
"BOOLEAN"               { return tok_Boolean; }
"DATE"                  { return tok_Date; }

"TRUE"                  { yylval.boolean_literal = true; return tok_True; }
"FALSE"                 { yylval.boolean_literal = false; return tok_False; }

"DECLARE"               { return tok_Declare; }
"printd"                { return tok_printd; }
"prints"                { return tok_prints; }
"IF"                    { return tok_If; }
"ELSE"                  { return tok_Else; }
"ENDIF"                 { return tok_End_If; }
"FOR"                   { return tok_For; }
"NEXT"                  { return tok_Next; }
"REPEAT"                { return tok_Repeat; }
"UNTIL"                 { return tok_Until; }
"WHILE"                 { return tok_While; }
"ENDWHILE"              { return tok_End_While; }
"PROCEDURE"             { return tok_Procedure; }
"ENDPROCEDURE"          { return tok_End_Procedure; }
"FUNCTION"              { return tok_Function; }
"ENDFUNCTION"           { return tok_End_Function; }
"RETURN"                { return tok_Return; }
"RETURNS"               { return tok_Returns; }
"CALL"                  { return tok_Call; }

[a-zA-Z][a-zA-Z0-9_]*   { yylval.identifier = strdup(yytext); return tok_Identifier; }
[0-9]+(\.[0-9]+)?       { yylval.double_literal = atof(yytext); return tok_Double_Literal; }
\"([^"]*)\"             {
    yylval.string_literal = strdup(yytext + 1);
    yylval.string_literal[strlen(yylval.string_literal) - 1] = '\0';
    return tok_String_Literal;
}

\'([^\\']|\\.)\' {
    yylval.char_literal = yytext[1];
    return tok_Char_Literal;
}

\/\/.*                  { /* Comment; ignore */ }
[ \t]+                  { /* Whitespace; ignore */ }

"+"|"-"|"*"|"/"|"="|";"|"("|")"|">"|"<"|"{"|"}"|"!"|"," { return yytext[0]; }
"<="                    { return tok_LE; }
">="                    { return tok_GE; }
"=="                    { return tok_EQ; }
"!="                    { return tok_NEQ; }
"++"                    { return tok_AddOne; }
"--"                    { return tok_SubOne; }
"AND"                   { return tok_And; }
"OR"                    { return tok_Or; }

.                       { yyerror("Illegal lexeme"); exit(EXIT_FAILURE); }

%%

int yywrap(void) {
    // At EOF, emit DEDENT tokens for any remaining indentation levels
    while (!indent_stack.empty()) {
        indent_stack.pop();
        return tok_DEDENT;
    }
    return 1;
}
