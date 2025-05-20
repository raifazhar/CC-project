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

// Debug function
void debug_token(const char* token_name, const char* value) {
    fprintf(stderr, "LEX: Line %d - Token: %s, Value: '%s'\n", yylineno, token_name, value ? value : "");
}

void debug_indent(int spaces, const char* action) {
    fprintf(stderr, "LEX: Line %d - Indentation: %d spaces, Action: %s\n", yylineno, spaces, action);
}
%}

%option yylineno
%option noyywrap

%x indent
%s normal

%%

<INITIAL>. {
    unput(*yytext);  /* Return the character to the input */
    indent_stack.push(0);  /* Initialize with zero indentation */
    start_of_line = true;
    BEGIN(indent);   /* Start in indent mode */
}

<normal>\n+ {
    debug_token("NEWLINE", "\\n");
    start_of_line = true;
    current_indent = 0;
    BEGIN(indent);
    return tok_Newline;
}

<indent>[ ] {
    if(start_of_line) current_indent++;
}

<indent>\t {
    if(start_of_line) current_indent = (current_indent + INDENT_WIDTH) & ~(INDENT_WIDTH-1);
}

<indent>\n+ {
    /* Empty line - reset indentation counter but keep state */
    debug_token("NEWLINE", "\\n");
    current_indent = 0;
    start_of_line = true;
    return tok_Newline;
}

<indent>[^ \t\n] {
    unput(*yytext);  /* Put back the non-whitespace character */
    debug_indent(current_indent, "Processing");
    
    int previous = indent_stack.empty() ? 0 : indent_stack.top();
    

    if (current_indent > previous) {
        debug_indent(current_indent, "INDENT");
        indent_stack.push(current_indent);
        start_of_line = false;
        BEGIN(normal);
        debug_token("INDENT", "");  /* Add token debug for indent */
        return tok_Indent;
    } 
    else if (current_indent < previous) {
        /* Generate DEDENT tokens */
        while (!indent_stack.empty() && current_indent < indent_stack.top()) {
            debug_indent(indent_stack.top(), "DEDENT");
            indent_stack.pop();
            dedent_buffer.push(tok_Dedent);
        }

        /* Check if indent levels match */
        if (!indent_stack.empty() && current_indent != indent_stack.top()) {
            fprintf(stderr, "ERROR: Inconsistent indentation at line %d\n", yylineno);
            yyerror("Inconsistent indentation");
        }
        
        if (!dedent_buffer.empty()) {
            int tok = dedent_buffer.front();
            dedent_buffer.pop();
            
            debug_token("DEDENT", "");  /* Add token debug for dedent */
            
            /* Stay in indent state if more dedents to process */
            if (dedent_buffer.empty()) {
                /* Only when all dedents are processed do we check
                   if we should switch to normal state */
                if (current_indent == indent_stack.top()) {
                    start_of_line = false;
                    BEGIN(normal);
                }
            }
            return tok;
        }
    }
    


    /* Same indentation level */
    start_of_line = false;
    BEGIN(normal);
}

<normal>"INTEGER"               { debug_token("INTEGER", yytext); return tok_Integer; }
<normal>"REAL"                 { debug_token("REAL", yytext); return tok_Real; }
<normal>"CHAR"                 { debug_token("CHAR", yytext); return tok_Char; }
<normal>"STRING"               { debug_token("STRING", yytext); return tok_String; }
<normal>"BOOLEAN"              { debug_token("BOOLEAN", yytext); return tok_Boolean; }
<normal>"DATE"                 { debug_token("DATE", yytext); return tok_Date; }

<normal>"TRUE"                 { debug_token("TRUE", yytext); yylval.boolean_literal = true; return tok_Bool_Literal; }
<normal>"FALSE"                { debug_token("FALSE", yytext); yylval.boolean_literal = false; return tok_Bool_Literal; }

<normal>"DECLARE"              { debug_token("DECLARE", yytext); return tok_Declare; }
<normal>"OUTPUT"               { debug_token("DECLARE", yytext); return tok_Output; }
<normal>"INPUT"               { debug_token("DECLARE", yytext); return tok_Input; }
<normal>"AND"                 { debug_token("AND", yytext); return tok_And; }
<normal>"OR"                  { debug_token("OR", yytext); return tok_Or; }
<normal>"NOT"                 { debug_token("NOT", yytext); return tok_Not; }
<normal>"IF"                   { debug_token("IF", yytext); return tok_If; }
<normal>"ELSE"                 { debug_token("ELSE", yytext); return tok_Else; }
<normal>"ENDIF"                { debug_token("ENDIF", yytext); return tok_End_If; }
<normal>"FOR"                  { debug_token("FOR", yytext); return tok_For; }
<normal>"TO"                   { debug_token("TO", yytext); return tok_To; }
<normal>"STEP"                 { debug_token("STEP", yytext); return tok_Step; }
<normal>"NEXT"                 { debug_token("NEXT", yytext); return tok_Next; }
<normal>"REPEAT"               { debug_token("REPEAT", yytext); return tok_Repeat; }
<normal>"UNTIL"                { debug_token("UNTIL", yytext); return tok_Until; }
<normal>"WHILE"                { debug_token("WHILE", yytext); return tok_While; }
<normal>"ENDWHILE"             { debug_token("ENDWHILE", yytext); return tok_End_While; }
<normal>"PROCEDURE"            { debug_token("PROCEDURE", yytext); return tok_Procedure; }
<normal>"ENDPROCEDURE"         { debug_token("ENDPROCEDURE", yytext); return tok_End_Procedure; }
<normal>"FUNCTION"             { debug_token("FUNCTION", yytext); return tok_Function; }
<normal>"ENDFUNCTION"          { debug_token("ENDFUNCTION", yytext); return tok_End_Function; }
<normal>"RETURN"               { debug_token("RETURN", yytext); return tok_Return; }
<normal>"RETURNS"              { debug_token("RETURNS", yytext); return tok_Returns; }
<normal>"CALL"                 { debug_token("CALL", yytext); return tok_Call; }
<normal>"ARRAY"                 { debug_token("ARRAY", yytext); return tok_Array; }
<normal>"OF"                 { debug_token("OF", yytext); return tok_Of; }

<normal>[a-zA-Z][a-zA-Z0-9_]*  { debug_token("IDENTIFIER", yytext); yylval.identifier = strdup(yytext); return tok_Identifier; }
<normal>[0-9]+             { debug_token("INTEGER", yytext); yylval.integer_literal = atoi(yytext); return tok_Integer_Literal; }
<normal>[0-9]+\.[0-9]+      { debug_token("REAL", yytext); yylval.real_literal = atof(yytext); return tok_Real_Literal; }
<normal>\"([^"]*)\"            {
    debug_token("STRING_LITERAL", yytext);
    yylval.string_literal = strdup(yytext + 1);
    yylval.string_literal[strlen(yylval.string_literal) - 1] = '\0';
    return tok_String_Literal;
}

<normal>\'([^\\']|\\.)\' {
    debug_token("CHAR_LITERAL", yytext);
    yylval.char_literal = yytext[1];
    return tok_Char_Literal;
}

<normal>\/\/.*                  { debug_token("COMMENT", yytext); /* Comment; ignore */ }
<normal>[ \t]+                  { /* General whitespace; ignore */ }

<normal>"++"                    { debug_token("INCREMENT", yytext); return tok_AddOne; }
<normal>"--"                    { debug_token("DECREMENT", yytext); return tok_SubOne; }
<normal>"<="                    { debug_token("LESS_EQUAL", yytext); return tok_LE; }
<normal>">="                    { debug_token("GREATER_EQUAL", yytext); return tok_GE; }
<normal>"=="                    { debug_token("EQUAL", yytext); return tok_EQ; }
<normal>"!="                    { debug_token("NOT_EQUAL", yytext); return tok_NEQ; }
<normal>"+"|"-"|"*"|"/"|"="|";"|"("|")"|">"|"<"|"{"|"}"|"!"|","|":"|"["|"]" { debug_token("SYMBOL", yytext); return yytext[0]; }



<normal>.                       { yyerror("Illegal lexeme"); exit(EXIT_FAILURE); }

<<EOF>> {
    if (current_indent > 0) {
        fprintf(stderr, "DEBUG: EOF with indentation level %d\n", current_indent);
        while (!indent_stack.empty()) {
            current_indent = indent_stack.top();
            indent_stack.pop();
            dedent_buffer.push(tok_Dedent);
        }
        current_indent = 0;
        
        if (!dedent_buffer.empty()) {
            int tok = dedent_buffer.front();
            dedent_buffer.pop();
            return tok;
        }
    }
    yyterminate();
}

%%