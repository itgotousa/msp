%{

%}

/* declare tokens */
%token  HEADER1 HEADER2 HEADER3 HEADER4 HEADER5 HEADER6 BOLD ITALIC LINK IMAGE LIST ITEM TEXT

%%
markdown:
    | markdown line
    ;

line    :   
    | header
    | bold
    | italic
    | link
    | image
    | list
    | text
    ;

header  :   HEADER TEXT { }


%%

