/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.hh"

int* background;

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_ambiguous = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void Command:: clear() {
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}


	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void Command::print() {
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void Command::execute() {

	if (_ambiguous) {
		printf("Ambiguous output redirect\n");
	}

	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
//	print();

	// Add execution here
	// For every simple command fork a new process
	int tmpin=dup(0);
	int tmpout=dup(1);
	int tmperr=dup(2);

	int fdin;
	if (_inFile) {
		fdin = open(_inFile, O_RDONLY);
	} else {
		fdin = dup(tmpin);
	}

	int ret;
	int fdout;
	int fderr;
	for ( int i = 0; i <_numOfSimpleCommands; i++) {
		dup2(fdin, 0);
		close(fdin);
		if (i == _numOfSimpleCommands - 1) {
			if (_outFile) {
				if (_append)
					fdout=open(_outFile, O_WRONLY | O_APPEND | O_CREAT, 0666 );
				else
					fdout=open(_outFile, O_WRONLY | O_TRUNC | O_CREAT, 0666 );
			} else {
				fdout=dup(tmpout);
			}
			if (_errFile) {
				if (_append)
					fderr=open(_errFile, O_WRONLY | O_APPEND | O_CREAT, 0666 );
				else
					fderr=open(_errFile, O_WRONLY | O_TRUNC | O_CREAT, 0666 );
			} else {
				fderr=dup(tmperr);
			}
		} else {
			int fdpipe[2];
			pipe(fdpipe);
			fdout = fdpipe[1];
			fdin = fdpipe[0];
		}

		dup2(fdout,1);
		close(fdout);
		dup2(fderr,2);
		close(fderr);

		if (!strcmp(_simpleCommands[i]->_arguments[0],"exit")) {
			printf("Good bye!\n");
			exit(1);
		} else if (!strcmp(_simpleCommands[i]->_arguments[0], "setenv")) {
			setenv(_simpleCommands[i]->_arguments[1],_simpleCommands[i]->_arguments[2],1);
			continue;
		} else if (!strcmp(_simpleCommands[i]->_arguments[0],"unsetenv")) {
			unsetenv(_simpleCommands[i]->_arguments[1]);
			continue;
		} else if (!strcmp(_simpleCommands[i]->_arguments[0],"cd")) {
			char **temp = environ;
			while(*temp) {
				if(!strncmp(*temp, "HOME",4))
					break;
				temp++;
			}
			char* home = (char*)malloc(strlen(*temp)-5);
			strcpy(home, *temp+5);

			int res;
			if (_simpleCommands[i]->_numOfArguments > 1)
				res = chdir(_simpleCommands[i]->_arguments[1]);
			else
				res = chdir(home);

			if (res != 0)
				perror("chdir");

			clear();
			prompt();
			return;
		} else {
			ret = fork();
		}
		if (ret == 0) {
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
			perror("execvp");
			_exit(1);
		} else if (ret < 0) {
			perror("fork");
			return;
		}
	}
	if (!_background) {
		waitpid(ret, NULL, 0);
	}
	// Setup i/o redirection
	// and call exec

	// Clear to prepare for next command
	clear();
	dup2(tmpin,0);
	dup2(tmpout,1);
	dup2(tmperr,2);
	// Print new prompt
	prompt();
}

// Shell implementation

void Command::prompt() {
	if (isatty(0)) {
		printf("myshell>");
		fflush(stdout);
	}
	
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;





