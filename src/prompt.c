#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <ncurses.h>
#include <assert.h>

#include "./prompt.h"

#define HOST_NAME_MAX 63
#define UID_MAX 31

#define UNUSED(x) (void)(x)

extern int errno;

/**
 * prompt.c 
 *
 * Contains I/O and REPL implementations.
 */

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
 * get_env
 *
 * Just the logic to retrieve things about current environment/ user
 *
 */
void get_env(char *uid, char *host, char *path, char *home){
  
  // Finds current euid to set bang and name according to
  struct passwd* p = getpwuid(geteuid());

  uid = (p) ? p->pw_name : "USER";

}


/**
 * Main REPL driving function.
 */
int wsh_main(int argc, char **argv) {
  // Passing in the given arguments allows easy
  // extensibility to be able to do e.g. "sh -c ls ~/Desktop"
  // or something like that.
  
  // these two are only used for passing in args
  UNUSED(argc);
  UNUSED(argv); 

  /** CLI stuff...
   * send argv directly over...
   */

  /**
   * Find hostname, username, and if current user is a root or not.
   * Does not support user swapping (command not built)
   */
  char* uid = "user";
  char* host = "wsh";
  char* path = "";
  char* home = "/";
  get_env(uid, host, path, home);

#ifdef PROMPT 
  char *bang = "user @ wsh % ";
  
  char hostname[HOST_NAME_MAX + 1];
  char *uid;
  struct passwd* p = getpwuid(geteuid());


  if (strlen(uid) > UID_MAX){ // theoretically never happens
    fprintf(stderr, "UID exceeds max length of %d; this is theoretically not possible on *NIX systems...", UID_MAX);
    uid = "USER";
    exit(1);
  } 

  int hn = gethostname(hostname, sizeof(hostname));
  if (hn < 0) {
    perror("Error retrieving hostname");
    exit(1);
  }

  bang = strcat(uid, "@");
  bang = strcat(bang, hostname);

  if(!geteuid()){
    bang = strcat(bang, " # ");
  } else {
    bang = strcat(bang, " % ");
  }

  // now, to get the current user home directory
  const char home_dir = p->pw_dir;


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
    int r = fprintf(stdout, "%s", bang); 
    if(r < 0) {
      perror("Issues printing out via printf. ");
      return 1;
    }
    int f = fflush(stdout);
    if (f < 0) {
      perror("Could not flush buffer properly");
      return 1; 
    }
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

    read(STDIN_FILENO, buf, sizeof(buf));
    parse(buf, tokens, args);

    //printf("\n Your input was %s\n", buf);
    printf("The first three tokens are %s, %s, %s", tokens[0], tokens[1], tokens[2]);
    printf("\n The first five args are \n");

    for (int i = 0; i < 5 && args[i]; i++) {
      printf("%s\n", args[i]);
    }
      
    

  }
#endif


  return 0; 
}
