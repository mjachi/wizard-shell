#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "ht.h"

// struct definitions for the hash table and node
// -- fields should be self-explanatory...

typedef struct HNode {
	const char *key;
  void *val;
} HNode;

typedef struct HashTable {
	HNode* entries;
	size_t size;
	size_t count;
} HashTable;
/**
 * A few "side" functions 
 */

// C2(3) is to have this nice function once finalized,
// but we're using C99.
//
// This is a "it" according to text 
// passed to my desktop over many a wire
// from many a server/ modem... you get the idea.
char *strdup(const char *str)
{
  char *str__;
  char *p;
  int len = 0;

  while (str[len])
      len++;
  str__ = malloc(len + 1);
  p = str__;
  while (*str)
      *p++ = *str++;
  *p = '\0';
  return str__;
}

// FNV simple hashing function implementation; 
// I know little of this sort of work, so this is ripped 
// from the internet.
uint64_t ht_hash(const char* key){
	assert(key);

	uint64_t hash = 14695981039346656037UL;
	for (const char* p = key; *p; p++) {
			hash ^= (uint64_t)(unsigned char)(*p);
			hash *= 1099511628211UL;
	}
	return hash;
}

// Helper for blowup and recycled in the "public" set function below
int ht_set_internal(HNode* hns, int capacity, size_t* plength, const char *key, void* val){
  assert(hns);
  assert(capacity > 50);
  assert(key);
  assert(val);

  uint64_t hash = ht_hash(key);
  int ind = (int)(hash & (uint64_t)(capacity-1));

  while(hns[ind].key) {
    if(!strcmp(key, hns[ind].key)){
      hns[ind].val = val;
      return 1;
    }
    ++ind;
    ind = ind > capacity - 1 ? 0 : ind;
  } // finds empty entry and places

  if (plength) { // special case I ran into...
    key = strdup(key);
    if (!key) {
      return 0;
    }
    (*plength)++;
  }
  hns[ind].key = (char *) key;
  hns[ind].val = val;

  return 1;
}

// Double table size if need be 
// factor obviously > 1
void ht_blowup(HashTable* ht, double factor){
	assert(ht);
  assert(factor > 1);

  size_t new_cap = (int) (factor*ht->size);
	HNode* new_nodes = calloc(new_cap, sizeof(HNode*));
	if(new_cap < ht->size || !new_nodes) {
		fprintf(stderr, "Overflowed on a HashTable resize...");
		exit(1);
	}

	// Now to move everything over
	for (size_t i = 0; i < ht->size; i++) {
		HNode curr = ht->entries[i];
		if (curr.key != NULL) {
			ht_set_internal(new_nodes, new_cap, 0, curr.key, curr.val);
		}
	}


	free(ht->entries);
	ht->entries = new_nodes;
	ht->size = new_cap;

}

/**
 * Hash Table member functions implementations
 */

// Create new table
HashTable* newHashTable(int size) {
	assert(size > 0);

	HashTable* ht = malloc(sizeof(HashTable));
	if(!ht) { // NULL will be used for error checking.
		return NULL;
	}
	
	ht->size = size;
	ht->entries = calloc(size, sizeof(HNode));
	if(!ht->entries) { // similar. need to free ht though.
		free(ht);
		return NULL;
	}

	return ht;
}


// Destroy existing table
//
// (No real case to return 0... greater issues exist
// if I hit a case where this fails)
int deleteHashTable(HashTable* ht){
	assert(ht); 

  // Free mem from each element
	size_t i;
	for (i = 0; i < ht->size; i++){
		if(ht->entries[i].key){
			free((void*)ht->entries[i].key);
		}
	}
  // Free mem from larger structs
	free(ht->entries);
	free(ht);

  return 1;
}

// Add an element to the hash table. Normal HT behavior.
int ht_set(HashTable* ht, char *key, void* val){
	assert(ht);
	assert(key);
	assert(val);

	if (ht->count >= ht->size) {
		ht_blowup(ht, 1.5);
    // adds half of the "current" size... hopefully keeps mem cost down long term
    // but will require more computation early
	}

  return ht_set_internal(ht->entries, ht->size, &ht->count, key, val);
}


// Retrieve an element from HT by key. Normal HT behavior.
void* ht_get(HashTable* ht, const char *key){
  assert(ht);
  assert(key);

  size_t i = (size_t) (ht_hash(key) & (uint64_t)ht->size-1); 
  // TODO -- does cast or bitwise cause issue?
  // idea is to take hash and cast to something we can index with
  
  while (ht->entries[i].key) {
    if (strcmp(ht->entries[i].key, key) == 0) {
      return ht->entries[i].val;
    }
    i = (++i > ht->size-1) ? 0 : i; // possibility of wrap-around
  }
  return NULL;
}

// may omit erase.........
// haven't worked out sizing back down.


// Returns the size (for convenience's sake later on)
//
// Personally am just in the habit of taking advantage of fields,
// but we'll see if I end up making use of this function either way
int ht_count(HashTable* ht) {
  return ht->count;
}


/**
 * C++ style iterator over entries array. Make usual runtime considerations if of concern.
 */

// "constructor"
// -- if known index for beginning, can be specified; most cases = zero, though.
hti ht_iter(HashTable* ht, size_t index) {
  
  hti iter;
  iter.ht = ht;
  iter.index = 0;

  return iter;
}

// iteration!
int ht_next(hti* iter) {
  
  while(iter->index < iter->ht->count){
    size_t i = (iter->index)++;
    if (iter->ht->entries[i].key) {
      HNode entry = iter->ht->entries[i];
      iter->curr_key = entry.key;
      iter->curr_val = entry.val;
      return 1;
    } // this might be NULL, but we want the next non null entry.
  }
  
  return 0; // when not found, returns 0
}





