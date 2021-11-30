#ifndef WIZARD_SHELL_PROMPT_H
#define WIZARD_SHELL_PROMPT_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
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
#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include <sys/utsname.h>

#include "history.h"
#include "completions.h"
#include "jobs.h"
#include "ht.h"
#include "builtin.h"

#define _BSD_SOURCE
#define _XOPEN_SORUCE 700
#define UNUSED(x) (void)(x)

extern int jcount;
extern job_list_t *jobs_list;


int wsh_main(int argc, char **argv);

#endif
