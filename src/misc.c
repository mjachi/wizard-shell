#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <ctype.h>

#include "misc.h"

#define UNUSED(x) (void)(x)

extern int errno;


/**
 * Contains a handful of random things... not much more than that
 * The code in here drives a lot of the quality of life features 
 * of wsh e.g. implementation for arithmetic evaluation in-line
 */ 

// 1 if is redirect token, 0 otherwise
int is_not_redirect(char *str) {
    return strcmp(str, "<") && strcmp(str, ">") && strcmp(str, ">>");
}

// finds first nonredirect and returns it
char *first_nonredirect(char *tokens[], char *prev) {
    if (is_not_redirect(tokens[0]) && is_not_redirect(prev)) {
        return tokens[0];
    } else {
        return first_nonredirect(tokens + 1, tokens[0]);
    }
}

// returns length of string array
int ppstrlen(char *alos[]) {
    int i;
    for (i = 0; alos[i] != NULL; i++) {
    }
    return i;
}

// returns the final string in a string array
char *ppstr_final(char *alos[]) {
    for (int i = 0; alos[i] != NULL; i++) {
        if (alos[i + 1] == NULL) {
            return alos[i];
        }
    }
    return NULL;
}


// notalpha. 1 when the passed string 
// has a nonalphabetical char, 0 otherwise
int notalpha(char *str) {
  int i;
  for (i = 0; i < strlen(str); i++) {
    if (!isalpha(str[i])) {
      return 1;
    }
  }
  return 0;
}

// 1 if the string is entirely lowercase.
// 0 otherwise.
//
// behavior not defined for non-alphabetical
// strings e.g. hidden files
int str_islower(char *str) {
  int len = strlen(str);
  int i;
  
  for (i = 0; i < len; i++) {
    if(str[i] != tolower(str[i])) {
      return 0;
    }
  }
  return 1;
}

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



/** Long term: algeval suite (the algeval#'s)
 *
 * This is the in-line "symbolic" algebra manipulation... namely, stores more complicated 
 * variables for expressions and hosts some generic functions/ operators e.g. trig functions
 * This + matheval basically adds on a small calculator. Typing is identical to matheval.
 *
 */


