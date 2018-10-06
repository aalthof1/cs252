#include <cstdlib>
#include <string.h>
#include "simpleCommand.hh"

SimpleCommand::SimpleCommand() {
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void SimpleCommand::insertArgument( char * argument ) {
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}

	char* temp = (char*)malloc(sizeof(char) * 1024);
	if (strchr(argument,'$') && strchr(argument,'{')) {
		int i = 0;
		int j = 0;
		while(argument[i]) {
			if(argument[j] == '$') {
				char* var = (char*) malloc(strlen(argument));
				i+=2;
				while(argument[i] != '}') {
					var[j++] = argument[i++];
				}
				var[j] = 0;
				strcat(temp, getenv(var));
				j=0;
			} else {
				char* var = (char*)malloc(strlen(argument));
				while(argument[i] && argument[i] != '$') {
					var[j++] = argument[i++];
				}
				var[j] = 0;
				strcat(temp,var);
				j=0;
				i--;
			}
			i++;
		}
		argument = strdup(temp);
	}
	
	_arguments[ _numOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numOfArguments + 1] = NULL;
	
	_numOfArguments++;
}
