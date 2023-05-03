%{
    #include "md.tab.h"
%}

%option noyywrap

%%

[ \t]*"#"{1}    { return HEADER1; }
[ \t]*"#"{2}    { return HEADER2; }
[ \t]*"#"{3}    { return HEADER3; }
[ \t]*"#"{4}    { return HEADER4; }
[ \t]*"#"{5}    { return HEADER5; }
[ \t]*"#"{6}    { return HEADER6; }
