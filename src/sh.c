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
 * Assumes a width of 80 characters for the shell.
 *
 */
int main(int argc, char **argv) {

  //debug = 0;
  //verbose = 0;
  
  //for (int i = 1; i < argc; i++) {
  //  if (!strcmp("-d", argv[i])){
  //    debug=1;
  //  }
  //  if (!strcmp("-v", argv[i])){
  //    verbose=1;
  //  }
  //}

  return (wsh_main(argc, argv));
}
