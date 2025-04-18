%{
	//Will be created by Bison later on but we will be creating it at the moment for testing.
	#include <llvm/IR/Value.h>
	#include "ssc.tab.h"
	#include <stdlib.h>
	#include <stdio.h>
	#include <string>
	extern void yyerror(const char *msg);
	
	//#define DEBUGSSC
	//This code is for producing debug output.
	#ifdef DEBUGSSC
		#define debugFlex(a) (printf("\n%d: Token=<ID, lexeme>=<%d, %s>\n",yylineno, a, yytext))
	#else
		#define debugFlex(a)
	#endif
%}

%option yylineno 
%option noyywrap

%%

"INTEGER"				{debugFlex(tok_INTEGER); return tok_integer;}
"REAL"					{debugFlex(tok_REAL); return tok_real;}
"CHAR"					{debugFlex(tok_CHAR); return tok_char;}
"STRING"				{debugFlex(tok_STRING); return tok_string;}
"BOOLEAN"				{debugFlex(tok_BOOLEAN); return tok_boolean;}
"DATE"					{debugFlex(tok_DATE); return tok_date;}

"TRUE"					{debugFlex(tok_TRUE); yylval.boolean_literal = true; return tok_TRUE;}
"FALSE"					{debugFlex(tok_FALSE); yylval.boolean_literal = false; return tok_FALSE;}

"printd"				{debugFlex(tok_printd); return tok_printd;}
"prints"				{debugFlex(tok_prints); return tok_prints;}
"if"                  	{debugFlex(tok_if); return tok_if;}
"else"                 	{debugFlex(tok_else); return tok_else;}
"for"                  	{debugFlex(tok_for); return tok_for;}
"return"               	{debugFlex(tok_return); return tok_return;}
"def"					{debugFlex(tok_def); return tok_def;}
[a-zA-Z][a-zA-Z0-9_]*	{debugFlex(tok_identifier); yylval.identifier=strdup(yytext); return tok_identifier;}
[0-9]+(\.[0-9]+)?		{debugFlex(tok_double_literal); yylval.double_literal=atof(yytext); return tok_double_literal;}
\"([^"]*)\" {debugFlex(tok_string_literal);
	yylval.string_literal = strdup(yytext + 1);
	yylval.string_literal[strlen(yylval.string_literal) - 1] = '\0'; 
    return tok_string_literal;
}
\'([^\\']|\\.)\' { 
    debugFlex(tok_char_literal);
    yylval.char_literal = yytext[1];  // Only the character inside the quotes
    return tok_char_literal;
}

\/\/.*					{;}
[ \n\r\t]				{;}
"+"|"-"|"*"|"\/"|"="|";"|"("|")"|">"|"<"|"{"|"}"|"!"|","    {debugFlex(yytext[0]); return yytext[0];}
"<=" { return tok_LE; }
">=" { return tok_GE; }
"=="  { return tok_EQ; }
"!="  { return tok_NEQ; }
"++" { return tok_AddOne; }
"--" { return tok_SubOne; }

.					{debugFlex(-1); yyerror("this is a illegal lexeme"); exit(EXIT_FAILURE);}
%%








