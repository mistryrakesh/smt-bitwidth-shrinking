%option noyywrap
%option yylineno

%{
  #include <stdio.h>
  #include <string.h>
  #include "readSmt.h"
  #include "readSmt.tab.h"
  using namespace std;

  extern YYSTYPE yylval;
  void yyerror(char *);
%}

whitespace [ \n\t]
t_int [0-9]
t_string [_a-zA-Z][_a-zA-Z0-9]*

t_const #b[0-1]+

%%

{whitespace}+              { /* ignore whitespaces */ }

\;.*                       { /* ignore comments */ }

"(set-logic".*             { /* ignore  */ }

"(set-info".*              { /* ignore  */ }

"(check-sat)"              { /* ignore  */ }

"(exit)"                   { /* ignore  */ }

"declare-fun"              { return DECLARE_FUN; }

"BitVec"                   { return BITVEC; }

"Bool"                     { return BOOL; }

"assert"                   { return ASSERT; }

"concat"                   { return CONCAT; }

"extract"                  { return EXTRACT; }

[\(\)_=]                   { return yytext[0]; }

{t_int}+                   { yylval.str = strdup(yytext); return INT; }

{t_const}                  { yylval.str = strdup(yytext); return CONSTANT; }

{t_string}                 { yylval.str = strdup(yytext); return STRING; }

.                          { yyerror(yytext); }

%%

void yyerror(char *s) {
  printf("[Warning] Unmatched character at line %d: [%s]\n", yylineno, s);
}
