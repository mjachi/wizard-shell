#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>

#include "./misc.h"

#define UNUSED(x) (void)(x)

extern int errno;


/**
 * Contains a handful of random things... not much more than that
 * The code in here drives a lot of the quality of life features 
 * of wsh e.g. implementation for arithmetic evaluation in-line
 */ 



/** matheval suite (the matheval#'s) -- TODO
 *
 * This is the in-line evaluation mentioned above... a few different functions are 
 * given below with the intent of them each returning different types. In the above 
 * name, replacing # with 
 *
 * -- i --> return integer
 * -- d --> return double
 * -- l --> return long
 */ 

int temp_for_compiler = 0;


/** Long term: algeval suite (the algeval#'s)
 *
 * This is the in-line "symbolic" algebra manipulation... namely, stores more complicated 
 * variables for expressions and hosts some generic functions/ operators e.g. trig functions
 * This + matheval basically adds on a small calculator. Typing is identical to matheval.
 *
 */


