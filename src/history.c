#include "history.h"

/**
 * Helper functions
 */

// if we reach history capacity, this just deletes the last node on the tree.
int pop_back(history* h){
  // should never happen
  if(h->first == NULL){
    return -1;
  }

  hist_node* curr = h->first;
  while(curr->next){
    curr = curr->next;
  } // iterate to the end;
  free(curr);
  return 1;
}

/**
 * Public functions, so to speak, follow
 */

// QoL for telling if history is blank
int h_isEmpty(history* h) {
  return h->count == 0;
}

// QoL for returning length of history
int h_length(history* h) {
  return h->count;
}


// just tags onto the front
int h_push(history* h, char* command) {
  if (!h) { // null history
    return -1;
  }

  ++h->count;

  if (h->count >= HIST_MAX) { // Delete the last node
    pop_back(h);
  }

  hist_node* new = malloc(sizeof(hist_node));
  if (!new) {
    return -1;
  }
  new->command = command;
  new->next = h->first;
  new->prev = NULL;

  if (h->first) {
    h->first->prev = new;
  }

  h->first = new;
  return 0;
}


// Recall using history struct to access and read, etc.



