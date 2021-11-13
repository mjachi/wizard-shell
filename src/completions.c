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

/**
 * Member functions.
 */ 

// Creates a new node
// Error check if returns NULL!
TrieNode *tn_newNode(void) {
  TrieNode *p = malloc(sizeof(struct TrieNode));
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
    for (int i = 0; i < strlen(key); i++){
        int ind = CHAR_TO_INDEX(key[i]);
        if (curr->children[ind] == NULL) {
            curr->children[ind] = tn_newNode();
        }
        curr = curr->children[ind];
    }
 
    // mark the current node as a leaf
    curr->isLeaf = 1;
}

// search through tree to tell when the key is there
// in whole
int tn_search (TrieNode *root, const char *key) {

    TrieNode* curr = root;
    for(int i = 0; i < strlen(key); i++) {
        int index = CHAR_TO_INDEX(key[i]);
        if (!curr->children[index]) {
            return 0;
        }
        curr = curr->children[index];
    }
 
    return (curr && curr->isLeaf);
}

// if no children
int isLastNode (TrieNode* root) {
    for (int i = 0; i < ALPHABET_S; i++) if (root->children[i]) return 0;
    return 1;
}

// return the prefix completion/ "suffix" so to speak
char *suggestionsRec (TrieNode *root, char *prefix) {
  if (isLastNode(root)) {
    return prefix;
  }

  int i;
  for (i = 0; i < ALPHABET_S; i++) {

    if (root->children[i]) {
      char c = i+(int)'a';
      char str[2];
      str[0] = c;
      str[1] = '\0';
      strcat(prefix, str);

      suggestionsRec(root->children[i], prefix);
    }
  }

  return NULL;
}





