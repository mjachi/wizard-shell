#ifndef WIZARD_SHELL_PROMPT_H
#define WIZARD_SHELL_PROMPT_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>

#include "history.h"
#include "completions.h"
#include "jobs.h"
#include "ht.h"
#include "builtin.h"

#define BUFF_SIZE 1024
#define TOKS 512

int wsh_main(int argc, char **argv);

#endif
