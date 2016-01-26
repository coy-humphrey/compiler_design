%{
// Coy Humphrey 
// Dummy parser for scanner project.
#include "lyutils.h"
%}

%debug
%defines
%error-verbose
%token-table
%verbose

%token ROOT

%token TOK_VOID TOK_BOOL TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_FALSE TOK_TRUE TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON

%token TOK_CHR TOK_ORD

%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_INITDECL
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD
%token TOK_DECLID TOK_FUNCTION TOK_PARAMLIST TOK_PROTOTYPE
%token TOK_VARDECL TOK_RETURNVOID TOK_NEWSTRING TOK_INDEX

%right TOK_IF TOK_ELSE
%right TOK_IFELSE
%right '='
%left  TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left  '+' '-'
%left  '*' '/' '%'
%right TOK_POS TOK_NEG '!' TOK_ORD TOK_CHR
%left '.' '['
%left VARIADIC
%right TOK_NEW
%right '('

%start start

%%

start      : program               { yyparse_astree = $1;  }
           ;
program    : program structdef     { $$ = adopt1 ($1, $2); }
           | program function      { $$ = adopt1 ($1, $2); }
           | program statement     { $$ = adopt1 ($1, $2); }
           | program error '}'     { free_ast ($3); $$ = $1; }
           | program error ';'     { free_ast ($3); $$ = $1; }
           |                       { $$ = new_parseroot(); }
           ;
           
                                   
structdef  : structroot '}'        { free_ast ($2); $$ = $1; }
           ;

structroot : structroot fielddecl ';' 
               { free_ast ($3); $$ = adopt1 ($1, $2); }
           
           | TOK_STRUCT TOK_IDENT '{' 
              { free_ast ($3); $2->symbol = TOK_TYPEID;
                $$ = adopt1 ($1, $2); }
           ;
           
fielddecl  : basetype TOK_IDENT { $2->symbol = TOK_FIELD;
                                  $$ = adopt1 ($1, $2); }
           | basetype TOK_ARRAY TOK_IDENT 
              { $3->symbol = TOK_FIELD; $$ = adopt2 ($2, $1, $3); }
           ;
           
basetype   : TOK_VOID    { $$ = $1; }
           | TOK_BOOL    { $$ = $1; }
           | TOK_CHAR    { $$ = $1; }
           | TOK_INT     { $$ = $1; }
           | TOK_STRING  { $$ = $1; }
           | TOK_IDENT   { $1->symbol = TOK_TYPEID; $$ = $1; }
           ;
           
function   : identdecl '(' ')' block
             {
                 free_ast ($3); 
                 astree* funct = new_astree (TOK_FUNCTION, $1->filenr,
                   $1->linenr, $1->offset, "");
                 $2->symbol = TOK_PARAMLIST; 
                 adopt2 (funct,$1, $2);
                 if ($4->symbol == ';') 
                 {
                    funct->symbol = TOK_PROTOTYPE;
                    free_ast ($4);
                 }
                 else adopt1 (funct, $4);
                 $$ = funct;
             } 
           | functionh ')' block
           { 
              free_ast ($2); 
              if ($3->symbol == ';') $1->symbol = TOK_PROTOTYPE;
              else adopt1 ($1, $3);
              $$ = $1; 
           }
           ;

functionh  : functionh ',' identdecl
             // $1->children[1] should be the paramlist
             { adopt1 ($1->children[1], $3); free_ast ($2); $$ = $1; }
           | identdecl '(' identdecl 
             { astree* funct = new_astree (TOK_FUNCTION, $1->filenr,
                 $1->linenr, $1->offset, "");
               $2->symbol = TOK_PARAMLIST; adopt1 ($2, $3);
               $$ = adopt2 (funct, $1, $2); }
           ;
           
identdecl  : basetype TOK_IDENT 
              { $2->symbol = TOK_DECLID; $$ = adopt1 ($1, $2); }
           | basetype TOK_ARRAY TOK_IDENT
              { $3->symbol = TOK_DECLID; $$ = adopt2 ($2, $1, $3); }
           ;
           
block      : ';'        { $$ = $1; }
           | blockh '}' { free_ast ($2); $$ = $1; }
           ;
           
blockh     : blockh statement { adopt1 ($1, $2); $$ = $1; }
           | '{' statement    
              { $1->symbol = TOK_BLOCK; adopt1 ($1, $2); $$ = $1; };
           ;
           
statement  : block    { $$ = $1; }
           | vardecl  { $$ = $1; }
           | while    { $$ = $1; }
           | ifelse   { $$ = $1; }
           | return   { $$ = $1; }
           | expr ';' { free_ast ($2); $$ = $1; }
           ;
           
vardecl    : identdecl '=' expr ';'
              { free_ast ($4);
                $2->symbol = TOK_VARDECL; $$ = adopt2 ($2, $1, $3); }
           ;
           
while      : TOK_WHILE '(' expr ')' statement 
             { free_ast2 ($2, $4); $$ = adopt2 ($1, $3, $5); }
           ;
           
ifelse     : TOK_IF '(' expr ')' statement
             %prec TOK_IF
             { free_ast2 ($2, $4); $$ = adopt2 ($1, $3, $5); }
           | TOK_IF '(' expr ')' statement TOK_ELSE statement
             %prec TOK_IFELSE
             { free_ast2 ($2, $4); free_ast ($6); 
               $1->symbol = TOK_IFELSE; adopt2 ($1, $3, $5);
               $$ = adopt1 ($1, $7); }
           ;
           
return     : TOK_RETURN ';' { free_ast ($2);
                              $1->symbol = TOK_RETURNVOID; $$ = $1; }
           | TOK_RETURN expr ';' { free_ast ($3); $$ = adopt1 ($1, $2);}
           ;
           
expr       : expr '+' expr { $$ = adopt2 ($2, $1, $3); }
           | expr '-' expr { $$ = adopt2 ($2, $1, $3); }
           | expr '*' expr { $$ = adopt2 ($2, $1, $3); }
           | expr '/' expr { $$ = adopt2 ($2, $1, $3); }
           | expr '%' expr { $$ = adopt2 ($2, $1, $3); }
           | expr '=' expr { $$ = adopt2 ($2, $1, $3); }
           | expr TOK_EQ expr { $$ = adopt2 ($2, $1, $3); }
           | expr TOK_NE expr { $$ = adopt2 ($2, $1, $3); }
           | expr TOK_LT expr { $$ = adopt2 ($2, $1, $3); }
           | expr TOK_GT expr { $$ = adopt2 ($2, $1, $3); }
           | expr TOK_LE expr { $$ = adopt2 ($2, $1, $3); }
           | expr TOK_GE expr { $$ = adopt2 ($2, $1, $3); }
           | '+' expr %prec TOK_POS
              { $1->symbol = TOK_POS; $$ = adopt1 ($1, $2); }
           | '-' expr %prec TOK_NEG
              { $1->symbol = TOK_NEG; $$ = adopt1 ($1, $2); }
           | '!' expr     { $$ = adopt1 ($1, $2); }
           | TOK_ORD expr { $$ = adopt1 ($1, $2); }
           | TOK_CHR expr { $$ = adopt1 ($1, $2); }
           | allocator { $$ = $1; }
           | call { $$ = $1; }
           | '(' expr ')' %prec '(' { free_ast2 ($1, $3); $$ = $2; }
           | variable { $$ = $1; }
           | constant { $$ = $1; }
           ;

           
allocator  : TOK_NEW TOK_IDENT '(' ')' %prec TOK_NEW
              { free_ast2 ($3, $4); $2->symbol = TOK_TYPEID;
                $$ = adopt1 ($1, $2); }
           | TOK_NEW TOK_STRING '(' expr ')' %prec TOK_NEW
              { free_ast2 ($3, $5); $1->symbol = TOK_NEWSTRING;
                free_ast ($2); $$ = adopt1 ($1, $4); }
           | TOK_NEW basetype '[' expr ']' %prec TOK_NEW
              { free_ast2 ($3, $5); $1->symbol = TOK_NEWARRAY;
                $$ = adopt2 ($1, $2, $4); }
           ;
           
call       : TOK_IDENT '(' ')' %prec VARIADIC { $2->symbol = TOK_CALL;
                                                $$ = adopt1 ($2, $1); }
           | callroot ')' %prec VARIADIC { free_ast ($2); $$ = $1; }
           ;

callroot   : callroot ',' expr %prec VARIADIC
              { free_ast ($2); $$ = adopt1 ($1, $3); }
           | TOK_IDENT '(' expr %prec VARIADIC 
              { $2->symbol = TOK_CALL; $$ = adopt2 ($2, $1, $3); }
           ;

variable   : TOK_IDENT { $$ = $1; }
           | expr '[' expr ']' %prec VARIADIC
             { free_ast ($4); $2->symbol = TOK_INDEX;
               $$ = adopt2 ($2, $1, $3); }
           | expr '.' TOK_IDENT  %prec VARIADIC
             { $3->symbol = TOK_FIELD; $$ = adopt2 ($2, $1, $3); }
           ;
           
constant   : TOK_INTCON     { $$ = $1; }
           | TOK_CHARCON    { $$ = $1; }
           | TOK_STRINGCON  { $$ = $1; }
           | TOK_FALSE      { $$ = $1; }
           | TOK_TRUE       { $$ = $1; }
           | TOK_NULL       { $$ = $1; }
           ;
%%

const char *get_yytname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}

