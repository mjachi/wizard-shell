#ifndef WIZARD_SHELL_BUILTIN_C
#define WIZARD_SHELL_BUILTIN_C

#include <stdio.h>
#include <unistd.h>
#include "prompt.h"

int bin_cd(int argc, char **argv);
int bin_ln(int argc, char **argv);
int bin_rm(int argc, char **argv);
int bin_jobs(int argc, char **argv);
int bin_fg(int argc, char **argv);
int bin_bg(int argc, char **argv);
int bin_cd(int argc, char **argv);


#endif
