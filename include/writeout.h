#ifndef WRITEOUT_H
#define WRITEOUT_H

#include <stdio.h>
#include <termios.h>
#include <unistd.h>

/**
 * Header for all I/O functionality
 */

void back_up_buffer(int n);
void clear_line_buffer(int len, int p);
int getch(void);
void whitespace(int n);

int print_arr(char **out);

#endif
