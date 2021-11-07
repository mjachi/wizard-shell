#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "prompt.c"

/**
 * The below is just a driving function that immediately calls into another 
 * function that has some greater strengths/ is able to do more than simply 
 * what is done here. This was done for the sake of organization.
 *
 * Assumes a width of 80 characters for the shell.
 *
 * Will eventually support 
 * - aliasing (distinct from linking; will happen)
 * - built-in networking tools (not sure what this entails)
 * - actual locale support (easy)
 * - news (probable)
 * - extensions (hopeful)
 * - built-in lynx/ links/ web browser (probable)
 * - built-in IRC (probable)
 * - built-in email client (hopeful)
 * - powerline fonts (more of a compatibility issue than anything)
 * - git inline (ie, see what branch + status, etc)
 * - greater terminal control/ take advantage of coloring and so on
 * - ... ?
 *
 * May cut some of those to be intentionally lightweight/ aiming for minimal
 *
 */

int main(int argc, char **argv) {
    return (wsh_main(argc, argv));
}
