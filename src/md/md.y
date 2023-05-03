%{

%}

/* declare tokens */
%token  SPACE NUMBER STAR DOT DASH PLUS BRACKETL BRACKETR
%token  PARENTHESESL PARENTHESESR CURLYBRACKETL CURLYBRACKETR
%token  ANGLEBRACKETL ANGLEBRACKETR COMMA EXCLAMATIONL EXCLAMATIONR
%token  HEADER1 HEADER2 HEADER3 HEADER4 HEADER5 HEADER6
%token  CODEMARK BOLD1 BOLD2 ITALICS1 ITALICS2 STRIKETHROUGH

%%
markdown:
    | markdown line
    ;

line    :   
        | header
        ;

header  :   header1 text { $$ = $2; }
        |   header2 text { $$ = $2; }
        |   header3 text { $$ = $2; }
        |   header4 text { $$ = $2; }
        |   header5 text { $$ = $2; }
        |   header6 text { $$ = $2; }
        ;                                

header1 :   HEADER1 text { $$ = $2; }
        ;
header2 :   HEADER2 text { $$ = $2; }
        ;
header3 :   HEADER3 text { $$ = $2; }
        ;
header4 :   HEADER4 text { $$ = $2; }
        ;
header5 :   HEADER5 text { $$ = $2; }
        ;
header6 :   HEADER6 text { $$ = $2; }
        ;

text    :   TEXT {}


%%

