
#include "builtin.h"


/*
 * Executes the cd Command on the given tokens
 *
 * Parameters:
 *  - tokens: the tokenized inputs to the command line
 *
 * Returns: Nothing
 */
int bin_cd(int argc, char **argv){
    if (argc < 2) {
        fprintf(stderr, "cd: syntax error -- requires an argument\n");
        return -1;
    }

    if (chdir(argv[1]) < 0) {
        perror("cd");
        return -1;
    }
    return 0; 
}

/*
 * Executes the ln Command on the given tokens
 *
 * Parameters:
 *  - tokens: the tokenized inputs to the command line
 *
 * Returns: Nothing
 */
int bin_ln(int argc, char **argv) {
  // Checks that there are at least 2 arguments after the ln string
  if (argc < 3) {
    fprintf(stderr, "wsh \n");
    return -1;
  }

  if (link(argv[1], argv[2]) < 0) {
    // If an error occurs in the execution, it is printed here
    perror("ln");
    return -1;
  }
  return 0;
}

/*
 * Executes the rm Command on the given tokens
 *
 * Parameters:
 *  - tokens: the tokenized inputs to the command line
 *
 * Returns: Nothing
 */
int bin_rm(int argc, char **argv) {
    // Checks that there's at least 1 arguments after the ln string
    if (argc < 2) {
        fprintf(stderr, "rm: syntax error\n");
        return -1;
    }

    if (unlink(argv[1]) < 0) {
        // If an error occurs in the execution, it is printed here
        perror("rm");
        return -1;
    }
    return 0;
}
