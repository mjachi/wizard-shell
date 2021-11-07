#ifndef WIZARD_SHELL_EXEC_H
#define WIZARD_SHELL_EXEC_H

#define INIT_ALIAS_MAX 50

#include "ht.h"

/***
 *
 * Below == list of what's implemented 
 * to be accessed externally.
 *
 */

int bin_splash(int argc, char **argv);
int bin_exit(int argc, char **argv);
int bin_rm(int argc, char **argv);
int bin_chdir(int argc, char **argv);
int bin_link(int argc, char **argv);
int bin_alias(int argc, char **argv);



// Creates the builtin hashtable
HashTable* gen_builtin();

// Creates a mostly empty hashtable for alias's
HashTable* get_alias();



#endif
