
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>
#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <string_val> WORD
%token NOTOKEN GREAT NEWLINE GREATGREAT LESS AMPERSAND GREATAMPERSAND GREATGREATAMPERSAND PIPE TWOGREAT

%{
//#define yylex yylex
#include <cstdio>
#include <string.h>
#include <pwd.h>
#include <dirent.h>
#include <regex.h>
#include <sys/types.h>
#include "command.hh"

void yyerror(const char * s);
int yylex();
void mustExpand(char* arg);
void expandWC(char* pre, char* post);

int numEnt = 0;
char** entries = NULL;
bool flag = false;

%}

%%

goal:
  commands
  ;

commands:
  command
  | commands command
  ;

command: simple_command
       ;

simple_command:	
  pipe_list iomodifier_list background_opt NEWLINE {
/*    printf("   Yacc: Execute command\n"); */
    Command::_currentCommand.execute();
  }
  | NEWLINE 
  | error NEWLINE { yyerrok; }
  ;

command_and_args:
  command_word argument_list {
    Command::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | command_and_args
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

argument:
  WORD {
/*    printf("   Yacc: insert argument \"%s\"\n", $1); */
	mustExpand($1);	
  }
  ;

command_word:
  WORD {
/*    printf("   Yacc: insert command \"%s\"\n", $1); */
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

background_opt:
  AMPERSAND {
/*    printf("   Yacc: set to run in background\n"); */
    Command::_currentCommand._background = 1;
  }
  | /* can be empty */
  ;

iomodifier_list:
  iomodifier_list iomodifier_opt
  |
  ;

iomodifier_opt:
  GREAT WORD {
    if (Command::_currentCommand._outFile != 0)
	Command::_currentCommand._ambiguous = 1;
/*    printf("   Yacc: insert output \"%s\"\n", $2); */
    Command::_currentCommand._outFile = $2;
  }
  | GREATGREAT WORD {
    if (Command::_currentCommand._outFile != 0)
	Command::_currentCommand._ambiguous = 1;
/*    printf("   Yacc: append output \"%s\"\n", $2); */
    Command::_currentCommand._outFile = $2;
    Command::_currentCommand._append = 1;
  }
  | LESS WORD {
    if (Command::_currentCommand._inFile != 0)
	Command::_currentCommand._ambiguous = 1;
/*    printf("   Yacc: get input \"%s\"\n", $2); */
    Command::_currentCommand._inFile = $2;
  }
  | GREATAMPERSAND WORD {
    if (Command::_currentCommand._outFile != 0)
	Command::_currentCommand._ambiguous = 1;
/*    printf("   Yacc: insert output and errors \"%s\"\n", $2); */
    Command::_currentCommand._outFile = $2;
    Command::_currentCommand._errFile = $2;
  }
  | GREATGREATAMPERSAND WORD {
    if (Command::_currentCommand._outFile != 0)
	Command::_currentCommand._ambiguous = 1;
/*    printf("   Yacc: append output and errors \"%s\"\n", $2);*/
    Command::_currentCommand._outFile = $2;
    Command::_currentCommand._errFile = $2;
    Command::_currentCommand._append = 1;
  }
  | TWOGREAT WORD {
/*    printf("   Yacc: redirect stderr to \"%s\"\n", $2); */
    Command::_currentCommand._errFile = $2;
  }
  | /* can be empty */ 
  ;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

void mustExpand(char* arg) {
	char* tmp = strdup(arg);
	if (!strchr(tmp,'*') && !strchr(tmp,'?') && !strchr(tmp,'~')) {
		Command::_currentSimpleCommand->insertArgument(arg);
		return;
	} else {
		expandWC("", tmp);
		if (numEnt >= 2) {
			for (int i = 0; i < numEnt-1; i++) {
				for(int j = 0; j < numEnt-i-1; j++) {
					if (strcmp(entries[j],entries[j+1]) > 0) {
						char* temp = entries[j];
						entries[j] = entries[j+1];
						entries[j+1] = temp;
					}
				}
			}
		}
		for (int i = 0; i < numEnt; i++) {
			Command::_currentSimpleCommand->insertArgument(entries[i]);
		}
		entries = NULL;
		numEnt = 0;
	}
}

void expandWC(char* pre, char* post) {
	if(post[0] == '/') {
		flag = true;
	} else if (post[0] == 0) {
		return;
	}
	char* dir = strchr(post, '/');
	char* comp = (char*)malloc(1024* sizeof(char));
	if(dir) {
		if(flag && !pre[0]) {
			post = dir+1;
			dir = strchr(post, '/');
			if (!dir) {
				strcpy(comp, post);
				comp[strlen(post)] = 0;
				post += strlen(post);
			} else {
				strncpy(comp, post, dir-post);
				comp[dir-post] = 0;
				post = dir + 1;
			}
			pre = "/";
		} else {
			strncpy(comp, post, dir-post);
			post = dir + 1;
		}
	} else {
		strcpy(comp, post);
		comp[strlen(post)] = 0;
		post += strlen(post);
	}
	char* tilde = (char*) malloc(sizeof(char) * 1024);
	if (comp[0] == '~') {
		struct passwd *pwd;
		if(!strcmp(comp, "~")) {
			pwd = getpwnam(getenv("USER"));
		} else {
			pwd = getpwnam(comp+1);
		}
		if (!pwd) {
			printf("Could not access user %s\n", comp);
		} else {
			char* tildeTemp = NULL;
			if(!post[0] && !pre[0]) {
				tildeTemp = pwd->pw_dir;
			} else if (!post[0]) {
				tildeTemp = strcat(pwd->pw_dir, comp);
			} else if (!pre[0]) {
				tildeTemp = strcat(pwd->pw_dir, post);
			}
			strcpy(tilde, tildeTemp);
			tilde[strlen(tildeTemp)] = 0;
			mustExpand(tilde);
			return;
		}
	}
	char* newPre = (char*)malloc(sizeof(char) * 1024);
	if(!strchr(comp,'*') && !strchr(comp,'?')) {
		char* newPreTemp = NULL;
		if(!strcmp(pre,"/")) {
			newPreTemp = strcat(strdup("/"),strdup(comp));
		} else if (!pre[0] && !flag) {
			newPreTemp = strdup(comp);
		} else {
			newPreTemp = strcat(strdup(pre),"/");
			newPreTemp = strcat(newPreTemp, comp);
		}
		newPre = strdup(newPreTemp);
		newPre[strlen(newPreTemp)] = 0;
		expandWC(newPre,post);
		return;
	}
	char* expr = (char*) malloc(sizeof(char) * (2*strlen(comp)));
	int count = 0;
	*expr = '^';
	expr++;
	while(*comp) {
		if(*comp == '?') {
			*expr = '.';
			expr++;
		} else if (*comp == '*') {
			*expr = '.';
			expr++;
			*expr = '*';
			expr++;
		} else if (*comp == '.') {
			*expr = '\\';
			expr++;
			*expr = '.';
			expr++;
		} else {
			*expr = *comp;
			expr++;
		}
		count++;
		comp++;
	}
	comp -= count;
	*expr = '$';
	expr++;
	*expr = '\0';
	expr++;
	regex_t reg;
	regmatch_t regMatch;
	int res = regcomp( &reg, expr, REG_EXTENDED|REG_NOSUB);
	if (res) {
		fprintf(stderr,"Bad regex");
		return;
	}	
	char* dirTemp;
	if(!pre[0])
		dirTemp = ".";
	else
		dirTemp = strdup(pre);
	DIR * directory = opendir(dirTemp);
	struct dirent * curr;
	if(!entries)
		entries = (char**)malloc(1024*sizeof(char*));
	int preLen = strlen(pre);
	curr = readdir(directory);
	while(curr) {
		if(!regexec(&reg, curr->d_name, 1, &regMatch, 0)) {
			if (numEnt >= 2) {
				for (int i = 0; i < numEnt-1; i++) {
					for(int j = 0; j < numEnt-i-1; j++) {
						if (strcmp(entries[j],entries[j+1])) {
							char* temp = entries[j];
							entries[j] = entries[j+1];
							entries[j+1] = temp;
						}
					}
				}
			}
			if (curr->d_name[0] == '.') {
				if (comp[0] == '.') {
					char* currTemp = NULL;
					if(!pre[0])
						currTemp = strdup(curr->d_name);
					else if (pre[0] == '/' && !pre[1])
						currTemp = strcat("/",strdup(curr->d_name));
					else {
						currTemp = strcat(strdup(pre),"/");
						currTemp = strcat(currTemp,curr->d_name);
					}
					newPre = strdup(currTemp);
					newPre[strlen(currTemp)] = 0;
					expandWC(newPre, post);
					if(!dir) {
						entries[numEnt] = strdup(newPre);
						numEnt++;
					}
				}
			} else {
				char* currTemp = "";
				if (!pre[0])
					currTemp = curr->d_name;
				else if(pre[0]=='/' && !pre[1])
					currTemp = strcat("/",curr->d_name);
				else {
					currTemp = pre;
					currTemp = strcat(currTemp,"/");
					currTemp = strcat(currTemp,curr->d_name);
				}
				newPre = strdup(currTemp);
				int len = preLen+1;
				while(currTemp[len] != '\0' && currTemp[len] != '/')
					len++;
				strncpy(pre,pre,preLen);
				if(preLen)
					pre[preLen]=0;
				newPre[len] = 0;
				expandWC(newPre, post);
				if(!dir) {
					entries[numEnt] = newPre;
					numEnt++;
				}
			}
		}
		curr = readdir(directory);
	}
	if (numEnt == 0) {
		numEnt++;
		char* temp = strcat(pre,comp);
		strcat(temp,post);
		entries[0] = temp;
	}
	closedir(directory); 
	flag = false;
}

#if 0
main()
{
  yyparse();
}
#endif
