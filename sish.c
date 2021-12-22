#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

/*
 * This program implements a command line interpreter or shell
 * Author: Joel Augustine
 */
void err_exit(char *msg) {
        perror(msg);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

	// defines the required variables
	const int MAX_ARGS = 10;
	const int MAX_CMDS = 10;
	const int MAX_HIST = 100;

	char *line = NULL;
	size_t len = 0;
        char *tokens[MAX_ARGS];
	char *commands[MAX_CMDS];
	char *hist[MAX_HIST];
	char *saveptr;
	char *savetokptr;
	int numToks = 0;
	int currentIndex = 0;
	int full = 0;
	int offset = -1;

        while (1) {
		//if not executing history [offset]
		if (offset == -1) {
			printf("sish> ");
			// use getline() to get the input string
	                getline(&line, &len, stdin);
			if (line == NULL)
				continue; //continue the loop if there is no input	
			line[strlen(line)-1] = '\0';
			
			//puts line into history array, freeing memory if full
			if (full)
				free(hist[currentIndex]);
			hist[currentIndex] = malloc(sizeof(line));
			strcpy(hist[currentIndex], line);
			currentIndex++;
			if (currentIndex == MAX_HIST) {
				currentIndex = 0;
				full = 1;
			}
		} else { //if executing history [offset]
			if (line != NULL)
				free(line);
			line = malloc(sizeof(hist[offset]));
			strcpy(line, hist[offset]);
			offset = -1;
		}
		//gets first command
		commands[0] = strtok_r(line, "|\n", &saveptr);
		// The input tokens are delimited by " "
		// gets the first token
	        tokens[0] = strtok_r(commands[0], " \n", &savetokptr);
		// if it is "exit" break from the loop
		if (tokens[0] == NULL)
			continue;
		if (strcmp(tokens[0], "exit") == 0) 
			break;

		// if  not, load the arguments into tokens
		numToks = 0;
		do {
			numToks++;
			tokens[numToks] = strtok_r(NULL, " \n", &savetokptr);
		} while (tokens[numToks] != NULL);
		
		//cd command
		if (strcmp(tokens[0], "cd") == 0) {
			if (chdir(tokens[1]) != 0)
				perror("chdir error");
			continue;
		}

		//history command
		if (strcmp(tokens[0], "history") == 0) {
			//no args
			if (tokens[1] == NULL) {
				int i = 0;
				int j;
	
				if (full) {
					for (j = currentIndex; j < MAX_HIST; j++) {
						printf("%d %s\n", i, hist[j]);
						i++;
					}
				}
				for (j = 0 ; j < currentIndex; j++) {
					printf("%d %s\n", i, hist[j]);
					i++;
				}
				continue;
			} 
			//clear history
			if (strcmp(tokens[1], "-c") == 0) {
				int i;
				for (i = 0; i < currentIndex; i++) 
					free(hist[i]);
				if (full)
					for (i = currentIndex; i < MAX_HIST; i++)
						free(hist[i]);
				full = 0;
				currentIndex = 0;
				continue;
			}
			//history [offset]
			//checks if offset is valid
			if (!isdigit(tokens[1][0])) {
				printf("invalid offset");
				continue;
			}
			offset = atoi(tokens[1]);
			if (offset < 0) {
				printf("offset out of range");
				offset = -1;
				continue;
			}

			if (full) {
				if (offset >= MAX_HIST) {
					printf("offset out of range");
					continue;
				}
				offset = (currentIndex + offset) % MAX_HIST;
			} else {
				if (offset >= currentIndex) {
					printf("offset out of range");
					continue;
				}
			}
			continue;

		}
		//parse the rest of the commands in the pipe; the first is already tokenized
		int numCommands = 0;
		do {
			numCommands++;
			commands[numCommands] = strtok_r(NULL, "|", &saveptr);
		} while (commands[numCommands] != NULL);
		//set pipefd array for all pipes in the command
		int pipefd[(numCommands-1)*2];
		int i;
		for (i = 0; i < numCommands-1; i++) {
                	if (pipe(pipefd + 2*i) == -1)
				err_exit("pipe error");
		}
		//fork and process each command		
		for (i = 0; i < numCommands; i++) {
			int cpid = fork();
			if (cpid == -1)
				err_exit("fork");
			if (cpid == 0) {
				if (i != 0)
					if (dup2(pipefd[(i-1)*2], STDIN_FILENO) != STDIN_FILENO)
						err_exit("dup2 error to stdin");

				if (i != numCommands-1)
					if (dup2(pipefd[2*i+1], STDOUT_FILENO) != STDOUT_FILENO)
						err_exit("dup2 error to stdout");
				int j;
				for (j = 0; j < (numCommands-1)*2; j++) 
					close(pipefd[j]);
				//tokenize commands; first one is already tokenized
				if (i > 0) {
					for (j = 0; j < numToks; j++) {
						tokens[j] = NULL;
					}
					
					tokens[0] = strtok_r(commands[i], " \n", &savetokptr);
					numToks = 0;
					do {
						numToks++;
						tokens[numToks] = strtok_r(NULL, " \n", &savetokptr);
					} while (tokens[numToks] != NULL);
				}
				execvp(tokens[0], tokens);
				err_exit("execvp failed");
			}
		}
		//close all pipes in parent
		for (i = 0; i < (numCommands-1)*2; i++)
			close(pipefd[i]);
		//wait for all children to complete
		int status;
		for (i = 0; i < numCommands; i++)
			wait(&status);
	}

	
	// free the memory allocated by getline
	free(line);

    	exit(EXIT_SUCCESS);
}



