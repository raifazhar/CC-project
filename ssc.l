%{
#include <llvm/IR/Value.h>
#include "ssc.tab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <stack>
#include <queue>

extern void yyerror(const char *msg);
extern int yylineno;

std::stack<int> indent_stack;
std::queue<int> dedent_buffer;
int current_indent = 0;
bool start_of_line = true;

#define INDENT_WIDTH 4
%}

%option yylineno
%option noyywrap

%%

^[ \t]*\n {
    start_of_line = true;
    // Empty or whitespace-only line; ignore
}

^[ \t]+ {
    if (!start_of_line) return 0;
    start_of_line = false;

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
        return tok_Indent;
    } else if (new_indent < current_indent) {
        while (!indent_stack.empty() && indent_stack.top() > new_indent) {
            indent_stack.pop();
            dedent_buffer.push(tok_Dedent);
        }
        current_indent = new_indent;
        if (!dedent_buffer.empty()) {
            int tok = dedent_buffer.front();
            dedent_buffer.pop();
            return tok;
        }
    }
    // If same indent level, do nothing
}

\n {
    start_of_line = true;
    return tok_Newline;
}


. {
    if (!dedent_buffer.empty()) {
        int tok = dedent_buffer.front();
        dedent_buffer.pop();
        return tok;
    }
    start_of_line = (*yytext == '\n');
    // Continue with token matching
    BEGIN(INITIAL);
    // Put your token rules below
}

"INTEGER"               { return tok_Integer; }
"REAL"                  { return tok_Real; }
"CHAR"                  { return tok_Char; }
"STRING"                { return tok_String; }
"BOOLEAN"               { return tok_Boolean; }
"DATE"                  { return tok_Date; }

"TRUE"                  { yylval.boolean_literal = true; return tok_Bool_Literal; }
"FALSE"                 { yylval.boolean_literal = false; return tok_Bool_Literal; }

"DECLARE"               { return tok_Declare; }
"printd"                { return tok_Printd; }
"prints"                { return tok_Prints; }
"IF"                    { return tok_If; }
"ELSE"                  { return tok_Else; }
"ENDIF"                 { return tok_End_If; }
"FOR"                   { return tok_For; }
"TO"                    { return tok_To; }
"STEP"                  { return tok_Step; }
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
[ \t]+                  { /* General whitespace; ignore */ }

"++"                    { return tok_AddOne; }
"--"                    { return tok_SubOne; }
"<="                    { return tok_LE; }
">="                    { return tok_GE; }
"=="                    { return tok_EQ; }
"!="                    { return tok_NEQ; }
"AND"                   { return tok_And; }
"OR"                    { return tok_Or; }
"+"|"-"|"*"|"/"|"="|";"|"("|")"|">"|"<"|"{"|"}"|"!"|","|":" { return yytext[0]; }

.                       { yyerror("Illegal lexeme"); exit(EXIT_FAILURE); }

%%
