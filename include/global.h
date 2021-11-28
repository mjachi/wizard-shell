#ifndef GLOBAL_H
#define GLOBAL_H

#include "jobs.h"

extern int jcount;
extern job_list_t *jobs_list;

typedef int (*bin_builtin)(int argc, char **argv);

#endif
