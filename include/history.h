#ifndef WIZARD_SHELL_HISTORY_H
#define WIZARD_SHELL_HISTORY_H

#include <stdlib.h>
#include <stddef.h>

// store 30 most recent commands
#define HIST_MAX 30

/**
 * History struct consists of 30 node long DLL that we parse through
 * with arrow keys in the history_dia function
 */ 

// define a DLL node for history data structure
typedef struct hist_node {
   char *command;
	
   struct hist_node *next;
   struct hist_node *prev;
} hist_node;


typedef struct history {
  hist_node* first; // the most recent command
  hist_node* curr; // keep track of when to move up, down

  int count;
} history;

// just if head is NULL, but abstracts away for readability
int h_isEmpty(history* h);

// find length of history DLL;
int h_length(history* h);

// insert at end
int h_push(history* h, char* command);

#endif
