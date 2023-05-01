%{
#include "svg_parser.h"
#include <stdio.h>
%}

%token OPEN_TAG CLOSE_TAG ATTR_NAME ATTR_VALUE

%%

svg_file: /* empty */
         | svg_file svg_element
         ;

svg_element: open_tag svg_content close_tag
           ;

open_tag: OPEN_TAG attributes
        ;

close_tag: CLOSE_TAG
         ;

attributes: /* empty */
          | attributes attribute
          ;

attribute: ATTR_NAME "=" ATTR_VALUE
         ;

svg_content: /* empty */
           | svg_content svg_element
           ;

%%

int main() {
    yyparse();
    return 0;
}

void yyerror(char* error_message) {
    fprintf(stderr, "%s\n", error_message);
}
