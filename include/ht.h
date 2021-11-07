#ifndef HT_H
#define HT_H

#include<stddef.h>

/**
 * Relatively naive implementation of a HashTable.
 * wsh uses this throughout for a lot of things; the 
 * table for builtins is probably the most frequently 
 * accessed. Makes implementation rather straightforward,
 * luckily.
 */

typedef struct HashTable HashTable;

// Now for declaring the member functions.

// Build a new HashTable with capacity as parameter.
// Recall not an actual capacity... will resize if 
// we surpass the specified size
//
// pointer on success, nullptr on fail
HashTable* newHashTable(int size);

// Delete a hash table... e.g. if not in use for a time
// 1 on success, 0 on failure
int deleteHashTable(HashTable* ht);

// Push a new element into the hash table
// 1 on success, 0 on failure
int ht_set(HashTable* ht, char *key, void* val);

// Delete from the table by key
// 1 on success, 0 on failure
int ht_erase(HashTable* ht, char *key);

// Retrieve a value from the table
void* ht_get(HashTable* ht, const char *key);

// Return the current size of the hash table
size_t ht_length(HashTable* ht);

// Struct for an iterator
typedef struct hti {
	const char *curr_key;
	void* curr_val;

	HashTable* ht;
	size_t index;
} hti;

// For iterating over the HT
int ht_next(hti* it);

#endif
