#ifndef WIZARD_SHELL_COMPLETIONS_H
#define WIZARD_SHELL_COMPLETIONS_H

#include <stdlib.h>
#include <string.h>

#define ALPHABET_S 32 // size of alphabet
#define ARR_SIZ(a) sizeof(a)/sizeof(a[0]) // QoL macro def's
#define CHAR_TO_INDEX(c) ((int)c - (int)'a') 

/**
 * Implementation of the Trie DS/ algorithms for the <Tab> completion system.
 */

typedef struct TrieNode TrieNode;

struct TrieNode {
  TrieNode *children[ALPHABET_S];
  int isLeaf; // 0 is has children, 1 if leaf
};

// getNode
//
// produces newnodes to be added. 
TrieNode *tn_getNode(void);

// inserts nodes into tree as appropriate
void tn_insert (TrieNode *root, const char *key);

// search through tree to tell if key is there
// Lets us know when to tell the user they need 
// to retype/ type on their own/ nothing to
// complete with, etc.
// in the usual sense, 0 if not there, 1 if there
int tn_search (TrieNode *root, const char *key);

// to remove
TrieNode* tn_remove(TrieNode *root, char *key, int depth);

/**
 * Other functions that should be globally accessible for this.
 *
 */

// function that will take in a single token 
// and search for possible completions. Prints the first 6
// options if multiple remain 
//
// token is what we search based on.
// inplace is set to the completion based as the user tabs
// through options or the only option is there is just one
// for the rest of one branch based on token.
char *completionOf(TrieNode *root, char *prefix);

#endif
