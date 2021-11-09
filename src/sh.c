#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ncurses.h>

#include "prompt.h"

/**
 * The below is just a driving function that immediately calls into another 
 * function that has some greater strengths/ is able to do more than simply 
 * what is done here. This was done for the sake of organization.
 *
 * Assumes a width of 80 characters for the shell.
 *
 */
int main(int argc, char **argv) {

  initscr();
  //refresh();
  int ret = wsh_main(argc, argv);
  endwin();
  return ret;
}
