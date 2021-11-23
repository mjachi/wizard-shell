#ifndef WRITEOUT_H
#define WRITEOUT_H

#include <stdio.h>
#include <termios.h>
#include <unistd.h>

/**
 * Header for doing all my I/O without curses
 *
 * No longer do I need to have issues with REPL based
 * programs e.g. vim and julia!
 */

void back_up_buffer(int);
void clear_line_buffer(int,int);
int getch();
void whitespace(int n);

#endif
