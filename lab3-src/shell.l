
/*
 *
 * CS-252 Fall 2017
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{
#include <unistd.h>
#include <cstring>
#include "y.tab.hh"
/*
extern "C" char* read_line();

int mygetc(FILE * f) {
	static char* p;
	char ch;
	
	if(!isatty(0))
		return getc(f);

	if(!p || !p[0]) {
		char* s = read_line();
		p = s;
	}

	ch = *p;
	p++;

	return ch;
}

#undef getc
#define getc(f) mygetc(f)
*/
static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" {
	return GREATGREAT;
}

"<" {
	return LESS;
}

"&" {
	return AMPERSAND;
}

">&" {
	return GREATAMPERSAND;
}

">>&" {
	return GREATGREATAMPERSAND;
}

"|" {
	return PIPE;
}

"2>" {
	return TWOGREAT;
}


\`[^\n\`]*\` {
	char * subs = strdup(yytext+1);
	subs[strlen(subs)-1] = '\0';
	 
}

[^ \t\n][^ \t\n]*  {
  /* Assume that file names have only alpha chars */
  yylval.string_val = strdup(yytext);
  return WORD;
}

"\""[^\n\t\\\"][^\n\t\\\"]*"\"" {
  yylval.string_val = strdup(yytext+1);
  yylval.string_val[yyleng-2] = '\0';
  return WORD;
}

