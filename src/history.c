#include "history.h"

/**
 * If the history capacity is reached, this deletes the last node
 * in the doubly linked list.
 *
 * Parameters:
 *  - h: the history struct to check
 *
 * Returns int based on success of operation (if no
 * error occurs, return 1; otherwise -1).
 */
int pop_back(history *h) {
  // should never happen
  if (h->first == NULL) {
    return -1;
  }

  hist_node *curr = h->first;
  while (curr->next) {
    curr = curr->next;
  } // iterate to the end;
  free(curr);
  return 1;
}

/**
 * Determines if the history is empty or not.
 *
 * Parameters:
 * - h: the history struct to check
 *
 * Returns int based on whether or not the history struct is empty.
 */
int h_isEmpty(history *h) { return h->count == 0; }

/**
 * Determines the current history length
 *
 * Parameters:
 * - h: the history struct to check
 *
 * Returns int representing the length of the history DLL
 */
int h_length(history *h) { return h->count; }

/**
 * Pushes a command string onto the history struct.
 *
 * Parameters:
 * - h: the history struct to check
 * - command: the command to push
 *
 * Returns int based on success of operation.
 */
int h_push(history *h, char *command) {
  if (!h) { // null history
    return -1;
  }

  ++h->count;

  if (h->count >= HIST_MAX) { // Delete the last node
    pop_back(h);
  }

  hist_node *new = malloc(sizeof(hist_node));
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
