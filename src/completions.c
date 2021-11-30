#include "completions.h"
#include <stdio.h>

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
// Returns new trie node
TrieNode *tn_getNode(void) {
    TrieNode *pNode = NULL;
 
    pNode = malloc(sizeof(struct TrieNode));
 
    if (pNode){
        int i;
 
        pNode->isLeaf = 0;
 
        for (i = 0; i < ALPHABET_S; i++)
            pNode->children[i] = NULL;
    }
    pNode->count = 0;
 
    return pNode;
}
 
// If not present, inserts
// If the key is prefix of trie node, just marks leaf node
void tn_insert(TrieNode *root, const char *key){
    int length = strlen(key), level, index;
 
    TrieNode *pCrawl = root;
 
    for (level = 0; level < length; level++) {
        index = CHAR_TO_INDEX(key[level]);
        if (!pCrawl->children[index]){
            pCrawl->children[index] = tn_getNode();
            pCrawl->count++;
        }
        pCrawl = pCrawl->children[index];
    }
 
    // mark last node as leaf
    pCrawl->isLeaf = 1;
}
 
// Returns 1 if key presents in trie, else 0
int tn_search(struct TrieNode *root, const char *key){
    int length = strlen(key), i, index;
    struct TrieNode *pCrawl = root;
 
    for (i = 0; i < length; i++) {
        index = CHAR_TO_INDEX(key[i]);
 
        if (!pCrawl->children[index]){
            return 0;
        }
 
        pCrawl = pCrawl->children[index];
    }
 
    return (pCrawl->isLeaf);
}

// Returns 1 if root has no children, else 0
int isEmpty(TrieNode* root){
    for (int i = 0; i < ALPHABET_S; i++)
        if (root->children[i])
            return 0;
    return 1;
}

// Recursive function to delete a key
TrieNode* tn_remove(TrieNode* root, char *key, int depth){
    if (!root){
      return NULL;
    }
 
    if (depth == strlen(key)) {
        if (root->isLeaf) {
            root->isLeaf = 0;
        }
        if (isEmpty(root)) {
            free(root); 
            root = NULL;
        }
        return root;
    }
    int index = key[depth] - 'a';
    root->children[index] =
          tn_remove(root->children[index], key, depth + 1);
 
    if (isEmpty(root) && root->isLeaf == 0) {
        free (root);
        return NULL;
    }
 
    return root;
}


// if no children
int isLastNode (TrieNode* root) {
    for (int i = 0; i < ALPHABET_S; i++) if (root->children[i]) return 0;
    return 1;
}

// returns the first completion found
char *suggestionsRec (TrieNode *root, char *prefix) {
  if (root->isLeaf) return prefix;
  if (isLastNode(root)) return prefix;
  
  for (int i = 0; i < ALPHABET_S; i++) {
    if (root->children[i]) {
      char str[2];
      str[0] = 97+i;
      str[1] = '\0';
      
      return suggestionsRec(root->children[i], strcat(prefix, str));
    }
  }
  return prefix; 
}


// return the prefix completion/ "suffix" so to speak
char *completionOf (TrieNode *root, char *prefix) {
  
  TrieNode *pCrawl = root;
  int len = strlen(prefix); 

  // traverse the tree as far as possible.
  for (int i = 0; i < len; i++) {
    int index = CHAR_TO_INDEX(prefix[i]);
    if (!pCrawl->children[index]) {
      return prefix;
    }
    pCrawl = pCrawl->children[index];
  }

  int is_leaf = (pCrawl->isLeaf);
  int is_last = isLastNode(pCrawl);
  
  if (is_leaf && is_last) { // word is completed.
    return prefix;
  } else if (!is_last) { // is not complete.
    return suggestionsRec(pCrawl, prefix);
  }

  return prefix;
}

