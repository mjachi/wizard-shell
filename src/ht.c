#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "ht.h"

/**
 * Defines generic hash table node
 */
typedef struct h_node {
  const char *key;
  void *val;
} h_node;

/**
 * Defines hash table node for builtin function tyep
 */
typedef struct b_node {
  const char *key;
  bin_builtin bi;
} b_node;

/**
 * Defines hash table for alias type
 */
typedef struct a_node {
  const char *key;
  char **value;
} a_node;

/**
 * Defines a generic hash table struct
 */
typedef struct hash_table {
  h_node *entries;
  size_t size;
  size_t count;
} hash_table;

/**
 * Defines a hash table struct for builtins
 */
typedef struct builtins_table {
  b_node *entries;
  size_t size;
  size_t count;
} builtins_table;

/**
 * Defines a hash table struct for alias's
 */
typedef struct alias_table {
  a_node *entries;
  size_t size;
  size_t count;
} alias_table;

/**
 * This is C23 stdlib but not C99
 *
 * Returns a pointer to a null-terminated byte string, which is a duplicate of
 * the string pointed to by *str.
 */
char *strdup(const char *str) {
  char *str__;
  char *p;
  size_t len = 0;

  while (str[len])
    len++;
  str__ = malloc(len + 1);
  p = str__;
  while (*str)
    *p++ = *str++;
  *p = '\0';
  return str__;
}

/**
 * Simple hashing function implementation
 */
uint64_t ht_hash(const char *key) {
  uint64_t hash = 14695981039346656037UL;
  for (const char *p = key; *p; p++) {
    hash ^= (uint64_t)(unsigned char)(*p);
    hash *= 1099511628211UL;
  }
  return hash;
}

// Helper for blowup and recycled in the "public" set function below
int ht_set_internal(h_node *hns, int capacity, size_t *plength, const char *key,
                    void *val) {

  uint64_t hash = ht_hash(key);
  int ind = (int)(hash & (uint64_t)(capacity - 1));

  while (hns[ind].key) {
    if (!strcmp(key, hns[ind].key)) {
      hns[ind].val = val;
      return 0;
    }
    ++ind;
    ind = ind > capacity - 1 ? 0 : ind;
  } // finds empty entry and places

  if (plength) { // special case
    key = strdup(key);
    if (!key) {
      return 1;
    }
    (*plength)++;
  }
  hns[ind].key = key;
  hns[ind].val = val;

  return 0;
}

// 1.5x table size if need be
// factor obviously > 1
void ht_blowup(hash_table *ht, double factor) {
  if (factor <= 1) {
    fprintf(stderr, "Factor of %f will not increase the HT", factor);
    return;
  }

  size_t new_cap = (size_t)(factor * ht->size);
  h_node *new_nodes = calloc(new_cap, sizeof(h_node *));
  if (new_cap < ht->size || !new_nodes) {
    fprintf(stderr, "Overflowed on a hash_table resize...");
    exit(1);
  }

  // Now to move everything over
  for (size_t i = 0; i < ht->size; i++) {
    h_node curr = ht->entries[i];
    if (curr.key != NULL) {
      ht_set_internal(new_nodes, new_cap, 0, curr.key, curr.val);
    }
  }

  free(ht->entries);
  ht->entries = new_nodes;
  ht->size = new_cap;
}

/**
 * Hash Table member functions implementations follow
 */

/**
 * Creates a new hash_table based on the parameter
 * size. Note that this is automatically realloc'd
 * in the case that too many elements are passed.
 *
 * Returns a pointer to the HT on success, and
 * NULL on fail.
 */
hash_table *ht_new_ht(int size) {

  if (size < 1) {
    fprintf(stderr, "size cannot be negative");
    return NULL;
  }

  hash_table *ht = malloc(sizeof(hash_table));
  if (!ht) { // NULL will be used for error checking.
    return NULL;
  }

  ht->size = size;
  ht->entries = calloc(size, sizeof(h_node));
  if (!ht->entries) { // similar. need to free ht though.
    free(ht);
    return NULL;
  }

  return ht;
}

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
int ht_delete_ht(hash_table *ht) {

  // Free mem from each element
  size_t i;
  for (i = 0; i < ht->size; i++) {
    if (ht->entries[i].key) {
      free((void *)ht->entries[i].key);
    }
  }
  // Free mem from larger structs
  free(ht->entries);
  free(ht);

  return 1;
}

/**
 * Push a new element with the given key/ value
 * pairs in the provided hash_table
 *
 * Returns 0 if it failed to insert and 1 if succeeded
 */
int ht_set(hash_table *ht, char *key, void *val) {

  if (ht->count >= ht->size) {
    ht_blowup(ht, 1.5);
    // adds half of the "current" size... hopefully keeps mem cost down long
    // term but will require more computation early
  }

  return ht_set_internal(ht->entries, ht->size, &ht->count, key, val);
}

/**
 * Retrieves a value from the given table
 * based on provided key string.
 *
 * Returns value on success and NULL when not found
 * or generic failure
 */
void *ht_get(hash_table *ht, const char *key) {

  size_t i = (size_t)(ht_hash(key) & (uint64_t)(ht->size - 1));
  // idea is to take hash and cast to something we can index with

  while (ht->entries[i].key) {
    if (strcmp(ht->entries[i].key, key) == 0) {
      return ht->entries[i].val;
    }
    i = (++i > ht->size - 1) ? 0 : i; // possibility of wrap-around
  }
  return NULL;
}

/**
 * Just return the hash_table count.
 * Assert is there to ensure non-null,
 * or we will have a seg-fault.
 */
int ht_count(hash_table *ht) { return ht->count; }

// Functions for the builtins table follow

/**
 * Full setting logic... separated into two functions for user interfacing
 * since this is required in the realloc'ing process
 */
int bt_set_internal(b_node *bns, int capacity, size_t *plength,
                    const char *name, bin_builtin bi) {

  uint64_t hash = ht_hash(name);
  int ind = (int)(hash & (uint64_t)(capacity - 1));

  while (bns[ind].key) {
    if (!strcmp(name, bns[ind].key)) {
      bns[ind].bi = bi;
      return 0;
    }
    ++ind;
    ind = ind > capacity - 1 ? 0 : ind;
  } // finds empty entry and places

  if (plength) { // special case
    name = strdup(name);
    if (!name) {
      return 1;
    }
    (*plength)++;
  }
  bns[ind].key = name;
  bns[ind].bi = bi;

  return 0;
}

/**
 * Resizing logic for when appropriate e.g. count exceeds the limited
 * size; naturally factor > 1, but is hard-coded to 1.5 below to try
 * to conserve. Notice exponential behavior if an extreme number of
 * K/V pairs are added.
 */
void bt_blowup(builtins_table *bt, double factor) {

  size_t new_cap = (size_t)(factor * bt->size);
  b_node *new_nodes = calloc(new_cap, sizeof(b_node *));
  if (new_cap < bt->size || !new_nodes) {
    fprintf(
        stderr,
        "{wsh @ builtins table} -- Overflowed on a builtins_table resize...");
    exit(1);
  }

  // Now to move everything over
  for (size_t i = 0; i < bt->size; i++) {
    b_node curr = bt->entries[i];
    if (curr.key != NULL) {
      bt_set_internal(new_nodes, new_cap, 0, curr.key, curr.bi);
    }
  }

  free(bt->entries);
  bt->entries = new_nodes;
  bt->size = new_cap;
}

// -- Beginning of accessible BT functions

/**
 * Creates a new builtins_table based on the parameter
 * size. Note that this is automatically realloc'd
 * in the case that too many elements are passed.
 *
 * Returns a pointer to the HT on success, and
 * NULL on fail.
 */
builtins_table *bt_new_bt(int size) {

  builtins_table *bt = malloc(sizeof(builtins_table));
  if (!bt) { // NULL will be used for error checking.
    return NULL;
  }

  bt->size = size;
  bt->entries = calloc(size, sizeof(b_node));
  if (!bt->entries) { // similar. need to free ht though.
    free(bt);
    return NULL;
  }

  return bt;
};

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
int bt_delete_bt(builtins_table *bt) {

  // Free mem from each element
  size_t i;
  for (i = 0; i < bt->size; i++) {
    if (bt->entries[i].key) {
      free((void *)bt->entries[i].key);
    }
  }
  // Free mem from larger structs
  free(bt->entries);
  free(bt);

  return 1;
}

/**
 * Push a new element with the given key/ value
 * pairs in the provided hash_table
 *
 * Returns 0 if it failed to insert and 1 if succeeded
 */
int bt_set(builtins_table *bt, char *name, bin_builtin bin) {

  if (bt->count >= bt->size) {
    bt_blowup(bt, 1.5);
    // adds half of the "current" size... hopefully keeps mem cost down long
    // term but will require more computation early
  }

  return bt_set_internal(bt->entries, bt->size, &bt->count, name, bin);
}

/**
 * Retrieves a value from the given table
 * based on provided key string.
 *
 * Returns value on success and NULL when not found
 * or generic failure
 */
bin_builtin bt_get(builtins_table *bt, const char *name) {

  size_t i = (size_t)(ht_hash(name) & (uint64_t)(bt->size - 1));
  // idea is to take hash and cast to something we can index with

  while (bt->entries[i].key) {
    if (strcmp(bt->entries[i].key, name) == 0) {
      return bt->entries[i].bi;
    }
    i = (++i > bt->size - 1) ? 0 : i; // possibility of wrap-around
  }
  return NULL;
}

/**
 * Returns the current of the given hash_table
 */
int bt_count(builtins_table *bt) { return bt->count; }

// End of builtins table implementations
// Functions for the alias table follow

/**
 * Full logic for setting the alias table values; required for both
 * the setting logic that is interfaced with elsewhere in addition
 * to resizing/ blowup below
 */
int at_set_internal(a_node *ans, int capacity, size_t *plength, const char *key,
                    char **val) {

  uint64_t hash = ht_hash(key);
  int ind = (int)(hash & (uint64_t)(capacity - 1));

  while (ans[ind].key) {
    if (!strcmp(key, ans[ind].key)) {
      ans[ind].value = val;
      return 0;
    }
    ++ind;
    ind = ind > capacity - 1 ? 0 : ind;
  } // finds empty entry and places

  if (plength) { // special case
    key = strdup(key);
    if (!key) {
      return 1;
    }
    (*plength)++;
  }
  ans[ind].key = key;
  ans[ind].value = val;

  return 0;
}

/**
 * Resizing logic for the alias_table.
 */
void at_blowup(alias_table *at, double factor) {

  size_t new_cap = (size_t)(factor * at->size);
  a_node *new_nodes = calloc(new_cap, sizeof(a_node *));
  if (new_cap < at->size || !new_nodes) {
    fprintf(stderr, "Overflowed on a hash_table resize...");
    exit(1);
  }

  // Now to move everything over
  for (size_t i = 0; i < at->size; i++) {
    a_node curr = at->entries[i];
    if (curr.key != NULL) {
      at_set_internal(new_nodes, new_cap, 0, curr.key, curr.value);
    }
  }

  free(at->entries);
  at->entries = new_nodes;
  at->size = new_cap;
}

// -- Accessible functions follow

/**
 * Creates a new alias_table based on the parameter
 * size. Note that this is automatically realloc'd
 * in the case that too many elements are passed.
 *
 * Returns a pointer to the AT on success, and
 * NULL on fail.
 */
alias_table *at_new_at(int size) {

  alias_table *at = malloc(sizeof(alias_table));
  if (!at) { // NULL will be used for error checking.
    return NULL;
  }

  at->size = size;
  at->entries = calloc(size, sizeof(a_node));
  if (!at->entries) { // similar. need to free ht though.
    free(at);
    return NULL;
  }

  return at;
}

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
 */
int at_delete_at(alias_table *at) {

  // Free mem from each element
  size_t i;
  for (i = 0; i < at->size; i++) {
    if (at->entries[i].key) {
      free((void *)at->entries[i].key);
    }
  }
  // Free mem from larger structs
  free(at->entries);
  free(at);

  return 1;
}

/**
 * Push a new element with the given key/ value
 * pairs in the provided hash_table
 *
 * Returns 0 if it failed to insert and 1 if succeeded
 */
int at_set(alias_table *at, char *abbr, char **tok_ext) {

  if (at->count >= at->size) {
    at_blowup(at, 1.5);
    // adds half of the "current" size... hopefully keeps mem cost down long
    // term but will require more computation early
  }

  return at_set_internal(at->entries, at->size, &at->count, abbr, tok_ext);
}

/**
 * Retrieves a value from the given table
 * based on provided key string.
 *
 * Returns value on success and NULL when not found
 * or generic failure
 */
char **at_get(alias_table *at, const char *abbr) {

  size_t i = (size_t)(ht_hash(abbr) & (uint64_t)(at->size - 1));
  // idea is to take hash and cast to something we can index with

  while (at->entries[i].key) {
    if (strcmp(at->entries[i].key, abbr) == 0) {
      return at->entries[i].value;
    }
    i = (++i > at->size - 1) ? 0 : i; // possibility of wrap-around
  }
  return NULL;
}

/**
 * Returns the current of the given hash_table
 */
int at_count(alias_table *at) { return at->count; }

// EOF
