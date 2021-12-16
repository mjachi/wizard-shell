#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "prompt.h"
#include "jobs.h"


/**
 * The below is just a driving function that immediately calls into another 
 * function that has some greater strengths/ is able to do more than simply 
 * what is done here. This was done for the sake of organization.
 *
 * Assumes a width of 80 characters for the terminal.
 *
 */
int main(int argc, char **argv) {
  int exit_code = 0;
#ifndef PROMPT
  exit_code = other_repl();
#else
  // Parent process signals.

  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  exit_code = wsh_main(argc, argv);
#endif
  return exit_code;
}
