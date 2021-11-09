#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "prompt.h"
#include "exec.h"

//#define _BSD_SOURCE
//#define _POSIX_SOURCE
#define UNUSED(x) (void)(x)
#define MAX_HOST 253
#define MAX_PATH 4096

extern int errno;

/**
 * prompt.c 
 *
 * Contains I/O and REPL implementations.
 */

/**
 * fileio
 *
 * Logic to manage redirection. 
 *
 */
void fileIO(char * args[], char* inputFile, char* outputFile, int option){
	 
	int err = -1;
  pid_t pid;
	
	int fileDescriptor; // between 0 and 19, describing the output or input file
	
	if((pid=fork())==-1){
		printf("Child process could not be created\n");
		return;
	}
	if(pid==0){
		// Option 0: output redirection
		if (option == 0){
			// We open (create) the file truncating it at 0, for write only
			fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600); 
			// We replace de standard output with the appropriate file
			dup2(fileDescriptor, STDOUT_FILENO); 
			close(fileDescriptor);
		// Option 1: input and output redirection
		}else if (option == 1){
			// We open file for read only (it's STDIN)
			fileDescriptor = open(inputFile, O_RDONLY, 0600);  
			// We replace de standard input with the appropriate file
			dup2(fileDescriptor, STDIN_FILENO);
			close(fileDescriptor);
			// Same as before for the output file
			fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
			dup2(fileDescriptor, STDOUT_FILENO);
			close(fileDescriptor);		 
		}
		 
		//setenv("parent",getcwd(currentDirectory, 1024),1);
		
		if (execvp(args[0],args)==err){
			printf("err");
			//kill(getpid(),SIGTERM);
		}		 
	}
	waitpid(pid,NULL,0);
}


/**
 * parse
 * -- Takes in a buffer, tokens, and argv. Breaks up the buffer and places
 *  accordingly into tokens/ argv inline
 * -- Produces a linked list of args for evaluation to be fed into run
 *
 * Intent is to make my life easier as far as pipes and more complicated execution goes.
 * Scalability, as they say...
 *
 * Buffer: char array for user input
 * Tokens: the tokenized inputs
 * Argv: the argument array eventually used for execv()
 *
 */
void parse(char buffer[BUFF_SIZE], char *tokens[TOKS], char *argv[TOKS]) {
  char whitespace[]= " \t";

  // produces the tokens array
  int argc = 0;
  char *tok = strtok(buffer, whitespace);
  while (tok) {
    tokens[argc++] = tok;
    tok = strtok(NULL, whitespace);
  }

  // takes tokens array and creates the argv array 
  /** argv array description
   * /bin/echo --> echo WLOG 
   *
   */
  int i = 0;
  while (tokens[i] && i < 512) {
    if (tokens[i][0] == '/') {
      char *last = strrchr(tokens[i], '/') + 1;
      argv[i] = last;
    } else {
      argv[i] = tokens[i];
    }
    i++;
  }
}


/**
 * Main REPL driving function.
 */
int wsh_main(int argc, char **argv) {
  UNUSED(argc);
  UNUSED(argv);

  /**
   * Find hostname, username, and if current user is a root or not.
   */
  char* t;
  char* uid = (t=getenv("USER")) ? t : "user";
  char host[MAX_HOST+1];
  if (gethostname(host, MAX_HOST+1) < 0) {
    endwin();
    perror("Issues retrieving hostname; seems to be undefined or long than the maximum length somehow");
    exit(1);
  }
  char* path = (t=getenv("PATH")) ? t : "/bin/";
  char* home = (t=getenv("HOME")) ? t : "/";
  char* cwd = home; // alternatively set to getcwd to start from 
                    // location of the executable
  
  /** CLI stuff... add conditional for a flag?
   * send argv directly over...
   */

#ifdef PROMPT 
  char *bang = "user @ wsh % ";

  //if(!geteuid()){
    //bang = strcat(bang, " # ");
  //} else {
    //bang = strcat(bang, " % ");
  //}

  /**
   * Opening script... prints out whatever is in mural.txt.
   * Assumes width of 80 characters... obviously, one could 
   * change whatever's in there to have it print that out 
   * instead
   */
  FILE *mural_ptr = fopen("mural.txt", "r");
  int c;
  if (mural_ptr != NULL) {
    while ((c = getc(mural_ptr)) != EOF){
        putchar(c);
    }
    fclose(mural_ptr);
    printf("wsh (:wizard: shell) v0.0.0\n\n\n");
  } else { // Generic error opening the mural.txt file
    fprintf(stderr, "Issue opening mural.txt file... skipping mural print...");
  }
  
  if (fflush(stdout) < 0) {
    perror("Could not flush buffer");
    return 1;
  }
 
  // Beginning of REPL
  for (;;) {

    // TODO -- print here

    /**
     * - User can enter nothing and hit the return key (your terminal should 
     *   simply ignore the input and start a new line in the terminal, as Linux systems do).
     * - User should enter a supported command if there is any non-whitespace text.
     * - User can enter a redirection symbol at ANY point in a command (see 4
     *   for details on redirection), as long as there is a valid file following it
     * - User cannot use any more than one of the same redirect in one line (i.e. one input, one output).
     * - User can enter any amount of whitespace between tokens
     */
    
    char buf[BUFF_SIZE];
    char *tokens[TOKS * sizeof(char *)];
    char *args[TOKS * sizeof(char *)];

    memset(buf, 0, sizeof(buf));
    memset(tokens, 0, sizeof(tokens));
    memset(args, 0, sizeof(args));

    //read(STDIN_FILENO, buf, sizeof(buf));
    getch();
    parse(buf, tokens, args);

    #ifdef DBG
      // file io to log
    #endif

  }
#endif


  return 0; 
}
