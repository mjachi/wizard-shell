#ifndef WIZARD_SHELL_MISC_H
#define WIZARD_SHELL_MISC_H

#include <string.h>


// 1 if is redirect token, 0 otherwise
int is_not_redirect(char *str);

// finds first nonredirect and returns it
char *first_nonredirect(char *tokens[], char *prev);

// returns length of string array
int ppstrlen(char *alos[]);

// returns the final string in a string array
char *ppstr_final(char *alos[]);

#endif
