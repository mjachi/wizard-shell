#ifndef HT_H
#define HT_H

#include <stddef.h>
#include "global.h"

/**
 * Relatively naive implementation of a hash_table.
 * builtins table and alias are typed specifically
 * for those use cases.
 */

typedef struct hash_table hash_table;
typedef struct builtins_table builtins_table;
typedef struct alias_table alias_table;

/**
 * Creates a new hash_table based on the parameter 
 * size. Note that this is automatically realloc'd
 * in the case that too many elements are passed.
 *
 * Returns a pointer to the HT on success, and 
 * NULL on fail.
 */
hash_table* ht_new_ht(int size);

/**
 * Frees the hash_table given to as parameter.
 *
 * Returns 1 always; if free fails, the program
 * will exit already, but this is indicative 
 * of greater issues at the OS level.
 *
 * Assertions are there simply because there 
 * should be never be an attempt to insert something
 * that is currently null.
 *
 */
int ht_delete_ht(hash_table* ht);

/**
 * Push a new element with the given key/ value
 * pairs in the provided hash_table
 *
 * Returns 0 if it failed to insert and 1 if succeeded
 */
int ht_set(hash_table* ht, char *key, void* val);

/**
 * Retrieves a value from the given table
 * based on provided key string.
 *
 * Returns value on success and NULL when not found
 * or generic failure
 */
void* ht_get(hash_table* ht, const char *key);

/**
 * Returns the current of the given hash_table
 */
int ht_count(hash_table* ht);

/**
 * Creates a new builtins_table based on the parameter 
 * size. Note that this is automatically realloc'd
 * in the case that too many elements are passed.
 *
 * Returns a pointer to the HT on success, and 
 * NULL on fail.
 */
builtins_table* bt_new_bt(int size);

/**
 * Frees the builtins_table given to as parameter.
 *
 * Returns 1 always; if free fails, the program
 * will exit already, but this is indicative 
 * of greater issues at the OS level.
 *
 * Assertions are there simply because there 
 * should be never be an attempt to insert something
 * that is currently null.
 *
 */
int bt_delete_bt(builtins_table* bt);

/**
 * Push a new element with the given key/ value
 * pairs in the provided hash_table
 *
 * Returns 0 if it failed to insert and 1 if succeeded
 */
int bt_set(builtins_table* bt, char *name, bin_builtin bin);

/**
 * Retrieves a value from the given table
 * based on provided key string.
 *
 * Returns value on success and NULL when not found
 * or generic failure
 */
bin_builtin bt_get(builtins_table* bt, const char *name);

/**
 * Returns the current of the given hash_table
 */
int bt_count(builtins_table* bt);

/**
 * Creates a new alias_table based on the parameter 
 * size. Note that this is automatically realloc'd
 * in the case that too many elements are passed.
 *
 * Returns a pointer to the HT on success, and 
 * NULL on fail.
 */
alias_table* at_new_at(int size);

/**
 * Frees the alias_table given to as parameter.
 *
 * Returns 1 always; if free fails, the program
 * will exit already, but this is indicative 
 * of greater issues at the OS level.
 *
 * Assertions are there simply because there 
 * should be never be an attempt to insert something
 * that is currently null.
 *
 */
int at_delete_at(alias_table* at);

/**
 * Push a new element with the given key/ value
 * pairs in the provided alias_table
 *
 * Returns 0 if it failed to insert and 1 if succeeded
 */
int at_set(alias_table* at, char *abbr, char **tok_ext);

/**
 * Retrieves a value from the given table
 * based on provided key string.
 *
 * Returns value on success and NULL when not found
 * or generic failure
 */
char** at_get(alias_table* at, const char *abbr);

/**
 * Returns the current of the given hash_table
 */
int at_count(alias_table* at);


#endif
