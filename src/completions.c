#include "completions.h"
#include <stdio.h>
#include <stdlib.h>

/** completions.c
 *
 * Contains the logic for the completions mechanism
 * as well as the Trie data structure and algo's implementation
 * the general specifications.
 *
 */


/**
 *
 * Creates and returns a freshly initialized TrieNode
 *
 * Returns a pointer to the new struct, given it is not null; in the case that it is,
 * this is taken as indicative of an error with malloc, so wsh exits cleanly.
 *
 */

TrieNode *tn_getNode(void) {
    TrieNode *pNode = NULL;

    pNode = malloc(sizeof(struct TrieNode));

    if (pNode){
        int i;

        pNode->isLeaf = 0;

        for (i = 0; i < ALPHABET_S; i++)
            pNode->children[i] = NULL;
    } else {
      // if pNode is null, something happened with malloc
      fprintf(stderr, "{wsh @ tn_getNode} -- Error on malloc;\
          was given a null pointer; exiting...");
      exit(1);
    }
    pNode->count = 0;

    return pNode;
}

/**
 *
 * Inserts a key into the trie node. More specifically, if the key
 * is not already present, then it is inserted; otherwise, marks the
 * corresponding node
 *
 * Parameters:
 * - root: the root trie node, denoting the trie to insert it into
 * - key: simply, the key to be inserted
 *
 * Nothing returned
 *
 */

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

/**
 *
 * Traverses the Trie beginning at root to search for the prefix; if
 * the key is complete in the trie, then we return 1 equiv true; false
 * otherwise
 *
 * Parameters:
 * - root: the TrieNode to begin traversal from
 * - key: the trie prefix to traverse by
 *
 * Returns 1 if the key is found and the final node traversed to is a leaf;
 * otherwise, return 0.
 *
 */

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

/**
 * Recursively traverses through the trie via the passed prefix/ key,
 * freeing as needed along the way
 *
 * Parameters:
 * - root: the TrieNode to begin traversal from
 * - key: the trie prefix/ key to traverse by
 * - depth: current depth in the trie, used to check whether or not
 *   the current node is a leaf in the trie, in which case it should be free'd
 *
 * Returns a TrieNode: at each level of recursion, this is whatever should replace
 * whatever may have been at that level in particular.
 *
 */

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

/**
 *
 * Simply indicates whether or not the passed node is at the bottom; this is the
 * case when the passed node has any children.
 *
 * Parameters:
 * - root: the TrieNode to check
 *
 * Returns 0 if root has no children; otherwise, returns 1 (for all i within the
 * alphabet, root's child i must be null)
 *
 */

int isLastNode (TrieNode* root) {
    for (int i = 0; i < ALPHABET_S; i++) if (root->children[i]) return 0;
    return 1;
}

/**
 *
 * Traverses the trie in search of, numerically, the first completion
 * to be found.
 *
 * Parameters:
 * - root: the TrieNode to begin traversal from
 * - prefix: the trie prefix to traverse by
 *
 * Returns the first suggestion found in the trie as a char array
 *
 */

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


/**
 *
 * Given a prefix, completionOf returns the first completion found in
 * the trie as a char array.
 *
 * Parameters:
 * - root: the TrieNode to begin traversal from; passed through several of
 *   the above functions
 * - prefix: the trie prefix to complete
 *
 * Returns a char array containing the prescribed completion
 *
 */

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

