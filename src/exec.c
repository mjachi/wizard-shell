#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <ncurses.h>

#include "exec.h"
#include "ht.h"

#define UNUSED(x) (void)(x)

extern int errno;
extern char **env;

/**
 * exec.c
 *
 * Contains the implementations of the different built-ins as well as the 
 * logic to run from $PATH, /bin/..., etc.
 */


/**  * * * * * * * *
 * BEGIN BUILTIN's *
 * * * * * * * * * */

/**
 * bin_splash
 *
 * just a "pretty"-fying function that prints out some things.
 */
int bin_splash(int argc, char **argv) {
  if (argc > 1) {
    // too many arguments
    return -1;
  }
  int ret = 0;
  FILE *mural_ptr = fopen("../mural.txt", "r");
  if (!mural_ptr) {
    printw("Skipping splash; failed to find mural.txt");
    ret = -1;
  }

  clear();
  int c;
  while ((c = getc(mural_ptr)) != EOF) {
    addch(c);
    refresh();
  }
  fclose(mural_ptr);
  printw("wsh (:wizard: shell) \n\n\n");

  refresh();
  
  return ret;
}


/**
 * bin_exit
 *
 * Contains the logic for exiting the shell. In short, steps down 
 * if forked/ is subshell, and terminates otherwise.
 *
 */
int bin_exit(int argc, char **argv) {
  UNUSED(argc);
  UNUSED(argv);
  
  // TODO --- forking/ subshell exiting.

  if (argc == 1) {
    endwin();
    exit(0);
  } else if (argc == 2) {
    endwin();
    exit(atoi(argv[1]));
  } 
  printw("exit: too many arguments passed. Recall \"exit <code>\", with the code being optional\n");
  refresh();
  return -1;
}

/** 
 * bin_rm
 *
 * Logic for the rm function... really a bin thing, but nonetheless
 *
 */
int bin_rm(int argc, char **argv) {
  UNUSED(argc);
	UNUSED(argv);


  return 0;
}


/**
 * bin_chdir
 *
 * builtin for file navigation. Hard-coded alias 
 * linking here from "cd home"
 *
 */
int bin_chdir(int argc, char **argv){
	UNUSED(argc);
	UNUSED(argv);

  return 0;
}

/**
 * bin_link
 *
 * builtin for linking
 *
 */
int bin_link(int argc, char **argv) {
	UNUSED(argc);
	UNUSED(argv);

	return 0;
}

/**
 * bin_alias
 *
 * Builtin for aliasing. If argc == 1, then 
 * prints out all the current aliases.
 *
 *
 */
int bin_alias(int argc, char **argv){
  return 0;
}

/**  * *
 * END *
 * * * */


//Next bit is just building the data structures for these...
// -- builtin's HT... iterate over the following list once to populate the HT

static void *bi_list[] = {
  "splash", &bin_splash,
  "rm", &bin_rm,
  "link", &bin_link,
  "chdir", &bin_chdir,
  "exit", &bin_exit
  // Keep going...
};

HashTable* gen_builtin(){
  int num_pairs = sizeof(bi_list) / (sizeof(bi_list[0]) + sizeof(bi_list[1]));

  HashTable* ht = newHashTable(num_pairs);

  // iterate through the above list;
  // hopefully it's clear what the intent was
  size_t i;
  for (i = 0; i < num_pairs*2; i+=2) {
    ht_set(ht, bi_list[i], bi_list[i+1]);
  }

  return ht;
}

// -- now the alias's HT

HashTable* gen_alias(){
  // initially allocate for arbitrary 50
  HashTable* ht = newHashTable(INIT_ALIAS_MAX);

  // hard code a few prebuilts...
  ht_set(ht, "cd", "chdir");

  return ht;
}


