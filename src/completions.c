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
 
    return pNode;
}
 
// If not present, inserts
// If the key is prefix of trie node, just marks leaf node
void tn_insert(TrieNode *root, const char *key){
    int length = strlen(key), level, index;
 
    TrieNode *pCrawl = root;
 
    for (level = 0; level < length; level++) {
        index = CHAR_TO_INDEX(key[level]);
        if (!pCrawl->children[index])
            pCrawl->children[index] = tn_getNode();
 
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
        if (root->isLeaf) root->isLeaf = 0;
        if (isEmpty(root)) free(root);
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
char *suggestionsRec (TrieNode *root, char *prefix, int app) {
  if (root->isLeaf) return prefix;
  if (isLastNode(root)) return prefix;

  for (int i = 0; i < ALPHABET_S; i++) {
    if (root->children[i]) {
      char str[2];
      str[0] = 97+i;
      str[1] = '\0';

      if (app) strcat(prefix, str);
      suggestionsRec(root->children[i], prefix, 1);
    }
  }
  return prefix; 
}


// return the prefix completion/ "suffix" so to speak
char *completionOf (TrieNode *root, char *prefix) {
  
  TrieNode *pCrawl = root;
  int len = strlen(prefix); 

  for (int i = 0; i+1 < len; i++) {
    int index = CHAR_TO_INDEX(prefix[i]);
    if (!pCrawl->children[index]) {
      return prefix;
    }
    pCrawl = pCrawl->children[index];
  }

  int is_leaf = (pCrawl->isLeaf);
  int is_last = isLastNode(pCrawl);

  if (is_leaf && is_last) {
    return prefix;
  } else if (!is_last) {
    return suggestionsRec(pCrawl, prefix,0);
  }

  return NULL;
}





