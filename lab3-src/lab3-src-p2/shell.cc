#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "command.hh"

int yyparse(void);

extern "C" void ctrlcHandler(int sig) {
	printf("\n");
	Command::_currentCommand.clear();
	Command::_currentCommand.prompt();
}

extern "C" void killZombies(int sig) {
	while(waitpid(-1,0,WNOHANG) > 0);
}


int main() {
	struct sigaction sa1;
	sa1.sa_handler = ctrlcHandler;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sa1, 0) == -1) {
		perror("sigaction");
		exit(1);
	}

	struct sigaction sa2;
	sa2.sa_handler = killZombies;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa2, 0) == -1) {
		perror("sigaction");
	}

	Command::_currentCommand.prompt();
	yyparse();
}
