#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "misc.h"

#define UNUSED(x) (void)(x)

extern int errno;

/**
 * Contains a handful of random things
 * The code in here drives a lot of the quality of life features
 * of wsh
 */

/**
 * Simple function determining whether an input is a redirection character
 * or not.
 *
 * Pair-wise logical AND on strcmp for the various redirect characters
 */
int is_not_redirect(char *str) {
  return strcmp(str, "<") && strcmp(str, ">") && strcmp(str, ">>");
}

/**
 * Finds the first non-redirect token and returns it
 */
char *first_nonredirect(char *tokens[], char *prev) {
  if (is_not_redirect(tokens[0]) && is_not_redirect(prev)) {
    return tokens[0];
  } else {
    return first_nonredirect(tokens + 1, tokens[0]);
  }
}

/**
 * Returns the length of a string array (ie char array array)
 */
int ppstrlen(char *alos[]) {
  int i;
  for (i = 0; alos[i] != NULL; i++)
    ;
  return i;
}

/**
 * Returns the final string in a string array (ie char array array)
 *
 */
char *ppstr_final(char *alos[]) {
  for (int i = 0; alos[i] != NULL; i++) {
    if (alos[i + 1] == NULL) {
      return alos[i];
    }
  }
  return NULL;
}

/**
 * Determins whether or not a char array (string) has a non
 * alpha-numeric character in it.
 *
 * Returns 1 when the passed string contains a non-alpha-numeric
 * character; 0 otherwise
 */
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

/**
 * Determines whether an input string is all lowercase (a helper for the
 * simplified completion system).
 *
 * Returns 1 when the input string is entirely lowercase; 0 otherwise.
 *
 * Not well-defined for non-alphabetical strings;
 * will error at `tolower()`
 */
int str_islower(char *str) {
  int len = strlen(str);
  int i;

  for (i = 0; i < len; i++) {
    if (str[i] != tolower(str[i])) {
      return 0;
    }
  }
  return 1;
}
