
#include "builtin.h"
#include <errno.h>
#include <string.h>

extern int errno;

/*
 * Executes the cd Command on the given tokens
 *
 * Parameters:
 *  - tokens: the tokenized inputs to the command line
 *
 * Returns: Nothing
 */
int bin_cd(int argc, char **argv){
    if (argc > 2) {
        printf("\n\tcd: syntax error -- too many arguments");
        return -1;
    }
    
    if (strcmp(argv[1], "~") == 0 || argc==1) {
      char *t = getenv("HOME");
      char *rst = strchr(t, '\n');
      if (!rst) {
        *rst = '\0';
      }
      int s = chdir(t);
      if (s < 0){
        char *error = strerror(errno);
        printf("\n\t cd: %s", error);
        return -1;
      }
    }

    if (chdir(argv[1]) < 0) {
        char *error = strerror(errno);
        printf("\n\t cd: %s", error);
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
    printf("\n\tln: syntax error -- requires arguments");
    return -1;
  }

  if (link(argv[1], argv[2]) < 0) {
    // If an error occurs in the execution, it is printed here
    char *error = strerror(errno);
    printf("\n\t ln: %s", error);
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
        printf("\n\trm: syntax error -- needs an argument");
        return -1;
    }

    for (int i = 1; i < argc; i++) {
      if (unlink(argv[1]) < 0) {
        char *error = strerror(errno);
        printf("\n\t rm: %s", error);
        if (argc == 1) {
          return -1;
        }
      }
    }
    return 0;
}

/**
 *
 * Executes the clear command on the given tokesn
 * 
 * Parameters:
 * - tokens: the tokenized inputs to the command line
 *
 * Returns an int based on the success/ failure of printing
 * the ANSI clear string provided.
 *
 */
int bin_clear(int argc, char **argv) {
  if (argc > 1) {
    printf("\n\tclear: syntax error -- too many arguments... takes none");
    return -1;
  }
  if (printf("\e[1;1H\e[2J")) {
    fprintf(stderr, "{wsh @ clear} -- ANSI clear string failed to print");
    return -1;
  }
  return 0;
}

