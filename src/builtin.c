#include "builtin.h"
#include "global.h"
#include <errno.h>
#include <string.h>

extern int errno;

/**
 * Executes the fg command on the given tokens
 *
 * Parameters:
 * - argc: the count of the tokens in argv
 * - argv: the tokenized inputs to the command line
 *
 * Returns int based on execution success or failure.
 */
int bin_fg(int argc, char **argv) {
  if (argc != 2) {
    printf("\n\tfg: syntax error -- requires exactly one argument");
    return -1;
  }

  if (argv[1][0] != '%') {
    printf("\n\tfg: job input does not begin with %%\n");
    return -1;
  }

  int jid = atoi(argv[1] + 1);
  pid_t pid = get_job_pid(jobs_list, jid);

  if (pid < 0) {
    printf("\n\tjob not found\n");
    return -1;
  }

  tcsetpgrp(STDIN_FILENO, pid);
  if (kill(-1 * pid, SIGCONT) < 0) {
    printf("\n\t{wsh @ bin_fg} -- error in continuing the job");
    return -1;
  }

  int wret, wstatus;
  if ((wret = waitpid(pid, &wstatus, WUNTRACED)) > 0) {
    if (WIFEXITED(wstatus)) {
      remove_job_jid(jobs_list, jid);
    }
    if (WIFSIGNALED(wstatus)) {
      remove_job_jid(jobs_list, jid);
      if (!printf("[%d] (%d) terminated by signal %d\n", jcount, wret,
                  WTERMSIG(wstatus))) {
        fprintf(stderr, "\n\t{wsh @ bin_fg} -- error writing to output\n");
      }
    }
    if (WIFSTOPPED(wstatus)) {
      update_job_jid(jobs_list, jid, STOPPED);
      if (!printf("[%d] (%d) suspended by signal %d\n", jcount, wret,
                  WSTOPSIG(wstatus))) {
        fprintf(stderr, "\n\t{wsh @ bin_fg} -- error writing to output\n");
      }
    }
  }

  tcsetpgrp(STDIN_FILENO, getpgrp());
  return 0;
}

/**
 * Executes the bg command on the given tokens
 *
 * Parameters:
 * - argc: the count of the tokens in argv
 * - argv: the tokenized inputs to the command line
 *
 * Returns int based on execution success or failure
 */
int bin_bg(int argc, char **argv) {
  // Checks that there's at least 1 arguments after the ln string
  if (argc != 2) {
    printf("\n\tbg: syntax error -- requires exactly one argument\n");
    return -1;
  }

  if (argv[1][0] != '%') {
    printf("\n\tbg: job input does not begin with %%\n");
    return -1;
  }

  int jid = atoi(argv[1] + 1);
  pid_t pid = get_job_pid(jobs_list, jid);

  if (pid < 0) {
    printf("\n\t{wsh @ bg} -- job not found with code %d\n", pid);
    return -1;
  }
  if (kill(-1 * pid, SIGCONT) < 0) {
    printf("\n\t{wsh @ bg} -- error in continuing the job");
    return -1;
  }

  return 0;
}

/**
 * Executes the jobs command on the given tokens
 *
 * Parameters:
 * - argc: token count in argv
 * - argv: the tokenized inputs to the command line.
 *
 * Returns: int based on execution success or failure
 */
int bin_jobs(int argc, char **argv) {
  if (argc != 1) {
    printf("\n\tjobs: syntax error -- too many arguments; doesn't take any\n");
    return -1;
  }
  if (jcount == 0) {
    printf("\n\tNo jobs to report");
    return 0;
  }
  jobs(jobs_list);
  return 0;
}

/*
 * Executes the cd command on the given tokens
 *
 * Parameters:
 *  - argc: the count of the tokens in argv
 *  - argv: the tokenized inputs to the command line
 *
 *  Name is for convention, but the raw tokens are fed in.
 *
 * Returns: int based on execution success or failure
 */
int bin_cd(int argc, char **argv) {
  if (argc > 2) {
    printf("\n\tcd: syntax error -- too many arguments");
    return -1;
  }

  if (argc == 1) {
    char *home = getenv("HOME");
    if (!home) {
      home = "/";
    }
    argv[1] = home;
  }

  if (chdir(argv[1]) < 0) {
    char *error = strerror(errno);
    printf("\n\t cd: %s", error);
    return -1;
  }
  return 0;
}

/*
 * Executes the ln command on the given tokens
 *
 * Parameters:
 *  - argc: the count of the tokens in argv
 *  - argv: the tokenized inputs to the command line
 *
 * Returns: Nothing
 */
int bin_ln(int argc, char **argv) {
  // Checks that there are at least 2 arguments after the ln string
  if (argc < 3) {
    printf("\n\tln: syntax error -- requires arguments");
    return -1;
  }

  if (link(argv[1], argv[2]) < 0) {
    // If an error occurs in the execution, it is printed here
    char *error = strerror(errno);
    printf("\n\t ln: %s", error);
    return -1;
  }
  return 0;
}

/*
 * Executes the rm command on the given tokens
 *
 * Parameters:
 *  - argc: token count in argv
 *  - argv: tokenized input
 *
 * Returns: int based on execution success or failure
 */
int bin_rm(int argc, char **argv) {
  // Checks that there's at least 1 arguments after the ln string
  if (argc < 2) {
    printf("\n\trm: syntax error -- needs an argument");
    return -1;
  }

  for (int i = 1; i < argc; i++) {
    if (unlink(argv[1]) < 0) {
      char *error = strerror(errno);
      printf("\n\t rm: %s", error);
      if (i == 1 && errno != 0) {
        return -1;
      }
    }
  }
  return 0;
}

/**
 *
 * Executes the clear command on the given tokesn, by printing
 * the ANSI clear command.
 *
 * Parameters:
 * - argc: token count in argv
 * - argv: the tokenized input
 *
 * Returns an int based on execution success/ failure.
 */
int bin_clear(int argc, char **argv) {
  if (argc > 1) {
    printf("\n\tclear: syntax error -- too many arguments... takes none");
    return -1;
  }
  if (printf("\e[1;1H\e[2J")) {
    fprintf(stderr, "{wsh @ clear} -- ANSI clear string failed to print");
    return -1;
  }
  return 0;
}

/**
 * Executes the exit command on the given tokens
 *
 * Parameters:
 * - argc: token count in argv
 * - argv: the tokenized input
 *
 * Exits before it returns anything.
 */
int bin_exit(int argc, char **argv) {
  int ec = 0;
  if (argc == 2) {
    ec = atoi(argv[1]);
    if (argv[1][0] != 48 && ec == 0) {
      fprintf(stderr, "\n\texit: syntax error -- seems something besides\
              an integer was passed as the argument");
      return -1;
    }
  } else if (argc > 2) {
    fprintf(stderr, "\n\texit: syntax error -- too many arguments");
    return -1;
  }

  cleanup_job_list(jobs_list);
  exit(ec);
}
