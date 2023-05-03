%{
#include <stdio.h>
#include <string.h>
#include "svg_parser.h"

// Declare yytext and yyleng as global variables
char* yytext = NULL;
int yyleng = 0;

// Declare a function to convert the current yytext to an integer
int get_integer() {
    return atoi(yytext);
}
%}

// Define regular expressions for the tokens
%option noyywrap

%{
    // Declare the yylex function
    int yylex();
%}

%%

"{" { ECHO; return LCURLY; }
"}" { ECHO; return RCURLY; }
"[" { ECHO; return LBRAC; }
"]" { ECHO; return RBRAC; }
"," { ECHO; return COMMA; }
":" { ECHO; return COLON; }
"true" { ECHO; return VTRUE; }
"false" { ECHO; return VFALSE; }
"null" { ECHO; return VNULL; }
[ \t\r\n]+ { /* eat whitespace */ }
\"[^"]*\" { yylval.string = strdup(yytext); ECHO; return STRING; }
[1-9]+\.?[0-9]*{EXP}? { ECHO; yylval.decimal = atof(yytext); return DECIMAL; }


<INITIAL>    \<          { yylval.tag = strdup(yytext); return OPEN_TAG; }
<INITIAL>    /\>         { return CLOSE_TAG; }
<INITIAL>    [a-zA-Z]+  { yylval.name = strdup(yytext); return ATTR_NAME; }
<INITIAL>    [-+]?[0-9]+ { yylval.value = get_integer(); return ATTR_VALUE; }
<INITIAL>    \s+         { /* Ignore whitespace */ }
<INITIAL>    .           { fprintf(stderr, "Unexpected character: %s\n", yytext); return 0; }
%%

// Define the yylex function
int yylex() {
    int token = yylex_destroyed ? 0 : yylex_internal();
    if (yytext != NULL) {
        free(yytext);
        yytext = NULL;
    }
    yyleng = 0;
    return token;
}

// Define a function to set the input string for the scanner
void set_input_string(const char* input_string) {
    yy_scan_string(input_string);
}
