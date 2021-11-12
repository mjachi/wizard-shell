#include "completions.h"

/** completions.c
 *
 * Contains the logic for the completions mechanism
 * as well as the Trie DS and algo's implementation 
 * the general specifications.
 *
 */

/***
 * Private functions, so to speak. Helpers, more accurately
 *
 */

// Returns the number of branches.
int count (TrieNode *children[]) {
  int c = 0, i;

  for (i = 0; i < ALPHABET_S; i++) {
    if(children[i]) {
      c++;
    }
  }

  return c;
}



/**
 * Member functions.
 */ 

// Creates a new node
// Error check if returns NULL!
TrieNode *tn_newNode(void) {
  TrieNode *p = (struct TrieNode *) malloc(sizeof(struct TrieNode));
  if (p) {
    int i;
    p->isLeaf = 0;
    for (i = 0; i < ALPHABET_S; i++) {
      p->children[i] = NULL;
    }
  }
  return p;
}

// inserts new word into the tree.
void tn_insert(TrieNode *root, const char *key){
    TrieNode *curr = root;
    while (*key){
        if (curr->children[*key - 'a'] == NULL) {
            curr->children[*key - 'a'] = tn_newNode();
        }
        curr = curr->children[*key - 'a'];
 
        key++;
    }
 
    // mark the current node as a leaf
    curr->isLeaf = 1;
}

// search through tree to tell when the key is there
// in whole
int tn_search (TrieNode *root, const char *key) {
    if (root == NULL) {
        return 0;
    }
    TrieNode* curr = root;
    while (*key)
    {
        curr = curr->children[*key - 'a'];
        if (curr == NULL) {
            return 0;
        }
        key++;
    }
 
    return curr->isLeaf;
}

// completion dialogue
void completion(TrieNode *root, char *token, WINDOW *w) {
  size_t tlen = strlen(token);
  
  TrieNode* curr = root;

  while(*token){
    curr = curr->children[*token-'a'];
  }

  return;
  
}




