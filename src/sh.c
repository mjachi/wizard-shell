#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ncurses.h>

#include "prompt.h"
#include "jobs.h"

// extern int debug;
// extern int verbose;
job_list_t *j_list;
int counter;

/*
 * Given a string, checks if it's not a redirection symbol
 *
 * Parameters:
 *  - str: the given string
 *
 * Returns:
 *  - 1 if str is not a redirection symbol, 0 if str is a redirection symbol
 */
int is_not_redirection(char *str) {
    return strcmp(str, "<") && strcmp(str, ">") && strcmp(str, ">>");
}

/*
 * parse()
 *
 * - Description: creates the token and argv arrays from the buffer character
 *   array
 *
 * - Arguments: buffer: a char array representing user input, tokens: the
 * tokenized input, argv: the argument array eventually used for execv()
 *
 * - Usage:
 *
 *      For the tokens array:
 *
 *      cd dir -> [cd, dir]
 *      [tab]mkdir[tab][space]name -> [mkdir, name]
 *      /bin/echo 'Hello world!' -> [/bin/echo, 'Hello, world!']
 *
 *      For the argv array:
 *
 *       char *argv[4];
 *       argv[0] = echo;
 *       argv[1] = 'Hello;
 *       argv[2] = world!';
 *       argv[3] = NULL;
 *
 * Returns: 0 if program exits correctly, 1 if there is an error
 */
int parse(char buffer[BUFF_SIZE], char *tokens[TOKS],
          char *argv[TOKS]) {
    // Get the token to the first part of the String:
    char *current_token;
    int tindex = 0;
    int aindex = 0;

    // Break the buffer into individual strings, delimited by any amount of
    // space Load each string into the token and argv
    while ((current_token = strtok(buffer, " \t\n")) != NULL) {
        tokens[tindex] = current_token;
        tindex++;
        // If the current token's not a re-direction symbol
        if (is_not_redirection(current_token)) {
            // Add the token to the argv
            if (strcmp(current_token, "&") != 0) {
                argv[aindex] = current_token;
                aindex++;
            }
        } else {
            // Load in the next token and only add it to the tokens array
            char *next_token = strtok(NULL, " \t\n");
            if (next_token == NULL) {
                return -1;
            }
            tokens[tindex] = next_token;
            tindex++;
        }

        buffer = NULL;
    }

    // Edit the header of argv to turn it into its appropriate form:
    if (argv[0] != NULL) {
        char *last_slash = strrchr(argv[0], '/');
        // If the last slash isn't a null pointer
        if (last_slash != NULL) {
            // Reset the first element to the string after the last slash
            argv[0] = last_slash + 1;
        }
    }

    // reaching here means no error occured
    return 0;
}

/*
 * Given an array of strings, counts the length of the string from the start
 * until finding the first null-pointer or when it reached the token limit
 *
 * Parameters:
 *  - alos: the list of strings
 *
 * Returns:
 *  - the length of the string until the first null
 */
int len_till_null(char *alos[]) {
    int i;
    for (i = 0; alos[i] != NULL && i < TOKS; i++) {
    }
    return i;
}

char *last_string(char *alos[]) {
    for (int i = 0; alos[i] != NULL && i < TOKS; i++) {
        if (alos[i + 1] == NULL) {
            return alos[i];
        }
    }
    return NULL;
}

/*
 * Executes the cd Command on the given tokens
 *
 * Parameters:
 *  - tokens: the tokenized inputs to the command line
 *
 * Returns: Nothing
 */
void cd_command(char *tokens[TOKS]) {
    // Checks that there are at least 1 argument after the cd string
    if (len_till_null(tokens) < 2) {
        fprintf(stderr, "cd: syntax error\n");
        return;
    }

    if (chdir(tokens[1]) < 0) {
        // If an error occurs in the execution, it is printed here
        perror("cd");
       return;
    }
}

/*
 * Executes the ln Command on the given tokens
 *
 * Parameters:
 *  - tokens: the tokenized inputs to the command line
 *
 * Returns: Nothing
 */
void ln_command(char *tokens[TOKS]) {
    // Checks that there are at least 2 arguments after the ln string
    if (len_till_null(tokens) < 3) {
        fprintf(stderr, "ln: syntax error\n");
        return;
    }

    if (link(tokens[1], tokens[2]) < 0) {
        // If an error occurs in the execution, it is printed here
        perror("ln");
        return;
    }
}

/*
 * Executes the rm Command on the given tokens
 *
 * Parameters:
 *  - tokens: the tokenized inputs to the command line
 *
 * Returns: Nothing
 */
void rm_command(char *tokens[TOKS]) {
    // Checks that there's at least 1 arguments after the ln string
    if (len_till_null(tokens) < 2) {
        fprintf(stderr, "rm: syntax error\n");
        return;
    }

    if (unlink(tokens[1]) < 0) {
        // If an error occurs in the execution, it is printed here
        perror("rm");
        return;
    }
}

void jobs_command(char *tokens[TOKS]) {
    // Checks that this is exactly the line "jobs"
    if (len_till_null(tokens) != 1) {
        fprintf(stderr, "jobs: syntax error\n");
        return;
    }

    jobs(j_list);
}

void fg_command(char *tokens[TOKS]) {
    // Checks that there's at least 1 arguments after the ln string
    if (len_till_null(tokens) != 2) {
        fprintf(stderr, "fg: syntax error\n");
        return;
    }

    if (tokens[1][0] != '%') {
        fprintf(stderr, "fg: job input does not begin with %%\n");
        return;
    }

    int jid = atoi(tokens[1] + 1);
    pid_t pid = get_job_pid(j_list, jid);
    if (pid < 0) {
        fprintf(stderr, "job not found\n");
        return;
    }

    tcsetpgrp(STDIN_FILENO, pid);
    if (kill(-1 * pid, SIGCONT) < 0) {
        fprintf(stderr, "ERROR in continuing the job");
        return;
    }

    int wret, wstatus;
    if ((wret = waitpid(pid, &wstatus, WUNTRACED)) > 0) {
        if (WIFEXITED(wstatus)) {
            // terminated normally
            remove_job_jid(j_list, jid);
        }
        if (WIFSIGNALED(wstatus)) {
            // terminated by a signal
            remove_job_jid(j_list, jid);
            if (fprintf(stdout, "[%d] (%d) terminated by signal %d\n", counter,
                        wret, WTERMSIG(wstatus)) < 0) {
                fprintf(stderr, "ERROR - Error Writing to Output\n");
            }
        }
        if (WIFSTOPPED(wstatus)) {
            // stopped
            update_job_jid(j_list, jid, STOPPED);
            if (fprintf(stdout, "[%d] (%d) suspended by signal %d\n", counter,
                        wret, WSTOPSIG(wstatus)) < 0) {
                fprintf(stderr, "ERROR - Error Writing to Output\n");
            }
        }
    }

    // waitpid(pid, &status, 0);
    // give terminal control back to shell

    // Giving the terminal control back to the Shell
    tcsetpgrp(STDIN_FILENO, getpgrp());
}

void bg_command(char *tokens[TOKS]) {
    // Checks that there's at least 1 arguments after the ln string
    if (len_till_null(tokens) != 2) {
        fprintf(stderr, "bg: syntax error\n");
        return;
    }

    if (tokens[1][0] != '%') {
        fprintf(stderr, "bg: job input does not begin with %%\n");
        return;
    }

    int jid = atoi(tokens[1] + 1);
    pid_t pid = get_job_pid(j_list, jid);
    if (pid < 0) {
        fprintf(stderr, "job not found\n");
        return;
    }

    if (kill(-1 * pid, SIGCONT) < 0) {
        fprintf(stderr, "ERROR in continuing the job");
        return;
    }
}

/*
 * Given a list of strings and the previous string before the start of the
 * list, finds the first string that isn't a redirection symbol and whose
 * previous string isn't a redirection symbol
 *
 * Parameters:
 *  - tokens: the tokenized inputs to the command line, this is the list that
 * will be search
 *  - prev: contains the previous string examined in this function, used for
 * recursion
 *
 * Returns: The First String satisfying the description given above.
 */
char *find_init(char *tokens[], char *prev) {
    // If the current string is not a redirection symbol, and its
    // previous string is not a redirection symbol:
    if (is_not_redirection(tokens[0]) && is_not_redirection(prev)) {
        return tokens[0];
    } else {
        // Recur on the next string in the list and pass the current string into
        // the argument for prev
        return find_init(tokens + 1, tokens[0]);
    }
}

/*
 * Given the tokens and argv from the input line, finds the relevant
 * redirection symbols and their associated files in the token and change the
 * standard in and out channels respectively, then execute argv into execv()
 *
 * Parameters:
 *  - tokens: the tokenized inputs to the command line
 *  - argv: the argument array eventually used for execv()
 *
 * Returns: Nothing
 */
void bin_command(char *tokens[TOKS], char *argv[TOKS],
                 int is_background) {
    // Initial Variables
    char *input = "\0";     // Input File
    char *s_output = "\0";  // Output File for ">"
    char *d_output = "\0";  // Output File for ">>"

    // Parsing through the tokens to find redirection symbols
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            // handle input redirection:
            // Check if the input variable has been changed already:
            if (strcmp(input, "\0") == 0) {
                input = tokens[i + 1];
                // Checks if the next file is NULL
                if (input == NULL) {
                    fprintf(stderr, "ERROR - No Input File Specified\n");
                    return;
                }
            } else {
                // If it is, then we have multiple of the same redirection
                // symbols
                fprintf(stderr, "syntax error: multiple input files\n");
                return;
            }

        } else if (strcmp(tokens[i], ">") == 0) {
            // handle output redirection:
            // Check if the s_output variable has been changed already:
            if (strcmp(s_output, "\0") == 0) {
                s_output = tokens[i + 1];
                if (s_output == NULL) {
                    fprintf(stderr, "ERROR - No Output File Specified\n");
                    return;
                }
            } else {
                // If it is, then we have multiple of the same redirection
                // symbols
                fprintf(stderr, "syntax error: multiple output files\n");
                return;
            }

        } else if (strcmp(tokens[i], ">>") == 0) {
            // handle double output redirection:
            // Check if the d_output variable has been changed already:
            if (strcmp(d_output, "\0") == 0) {
                d_output = tokens[i + 1];
                if (d_output == NULL) {
                    fprintf(stderr, "ERROR - No Output File Specified\n");
                    return;
                }
            } else {
                // If it is, then we have multiple of the same redirection
                // symbols
                fprintf(stderr, "syntax error: multiple output files\n");
                return;
            }
        }
    }

    // If both the > and >> appears in the token, then we signal an error
    if (strcmp(s_output, "\0") != 0 && strcmp(d_output, "\0") != 0) {
        fprintf(stderr, "syntax error: multiple output files\n");
        return;
    }

    // Otherwise, we specify what the outputfile is and whether to use > or >>
    int is_output_double =
        (strcmp(d_output, "\0") != 0);  // Whether it's > or >> for output file
    char *output = (is_output_double) ? d_output : s_output;  // Output File

    pid_t pid = 0;

    if (!(pid = fork())) {
        /* now in child process */
        // Setting Process IDs
        pid = getpid();
        // // pid_t pgid = getpgid(pid);
        setpgid(pid, pid);
        if (!is_background) {
            //     // This is a Foreground Process:
            tcsetpgrp(STDIN_FILENO, pid);
        }

        // Setting Up Signal Ignores in Parent:
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCONT, SIG_DFL);

        // Handling Redirections
        if (strcmp(input, "\0") != 0) {
            // First we close the stdin (0)
            if (close(STDIN_FILENO) < 0) {
                fprintf(stderr, "ERROR - Error Closing Standard Input\n");
                exit(1);
            }

            // Then we open the input file as the new stdin, with read only
            // permission
            if (open(input, O_RDONLY) < 0) {
                fprintf(stderr, "ERROR - Error Opening Input File\n");
                exit(1);
            }
        }

        if (strcmp(output, "\0") != 0) {
            // First we close the stdout (0)
            if (close(STDOUT_FILENO) < 0) {
                fprintf(stderr, "ERROR - Error Closing Standard Output\n");
                exit(1);
            }

            if (is_output_double) {
                // Opening for >> symbol
                // We open the output file as the new stdout, with relevant
                // permissions for >>
                if (open(output, O_RDWR | O_CREAT | O_APPEND, 00600) < 0) {
                    fprintf(stderr, "ERROR - Error Opening Output File\n");
                    exit(1);
                }
            } else {
                // Opening for > symbol
                // We open the output file as the new stdout, with relevant
                // permissions for >
                if (open(output, O_RDWR | O_CREAT | O_TRUNC, 00600) < 0) {
                    fprintf(stderr, "ERROR - Error Opening Output File\n");
                    exit(1);
                }
            }
        }

        // Executes the command in the child process
        execv(find_init(tokens, "\0"), argv);

        /* we won’t get here unless execv failed */
        perror("execv");

        // Exit Safely to end the Child Process
        exit(1);
    }

    if (is_background) {
        counter = counter + 1;
        add_job(j_list, counter, pid, RUNNING, find_init(tokens, "\0"));
        if (fprintf(stdout, "[%d] (%d)\n", counter, pid) < 0) {
            fprintf(stderr, "ERROR - Error Writing to Output\n");
        }

    } else {
        // fprintf(stdout, "Foreground\n");

        int wret, wstatus;
        if ((wret = waitpid(pid, &wstatus, WUNTRACED)) > 0) {
            if (WIFEXITED(wstatus)) {
                // terminated normally
                // remove_job_pid(j_list, wret);
                // fprintf(stdout, "Foreground terminates\n");
                // if (fprintf(stdout, "[%d] (%d) terminated with exit status
                // %d\n", wjid, wret, WEXITSTATUS(wstatus)) < 0) {
                //     fprintf(stderr, "ERROR - Error Writing to Output\n");
                // }
            }
            if (WIFSIGNALED(wstatus)) {
                // terminated by a signal
                counter = counter + 1;
                if (fprintf(stdout, "[%d] (%d) terminated by signal %d\n",
                            counter, wret, WTERMSIG(wstatus)) < 0) {
                    fprintf(stderr, "ERROR - Error Writing to Output\n");
                }
            }
            if (WIFSTOPPED(wstatus)) {
                // stopped
                counter = counter + 1;
                add_job(j_list, counter, pid, STOPPED, find_init(tokens, "\0"));
                if (fprintf(stdout, "[%d] (%d) suspended by signal %d\n",
                            counter, wret, WSTOPSIG(wstatus)) < 0) {
                    fprintf(stderr, "ERROR - Error Writing to Output\n");
                }
            }
        }

        // waitpid(pid, &status, 0);
        // give terminal control back to shell

        // Giving the terminal control back to the Shell
        tcsetpgrp(STDIN_FILENO, getpgrp());
    }
}

/*
 * Main function:
 * Initiates the REPL Loop until being terminated.
 *
 * Parameters: None
 *
 * Returns:
 *  - 0 if program exits correctly, 1 if there is an error
 */
int wmain() {
    // Setting up the Initial Variables for the Main Function
    char buf[BUFF_SIZE];
    char *tokens[TOKS];
    char *execv[TOKS];
    ssize_t count;

    // Initializes the Variables for Parsing
    memset(buf, '\0', BUFF_SIZE);

    // Set up Job List
    j_list = init_job_list();
    counter = 0;

    char *header = "wsh % ";
    int header_length = strlen(header);

    // Initial Write
#ifdef PROMPT
    // Starts with the header "33sh> "
    if (write(1, header, header_length) < 0) {
        fprintf(stderr, "ERROR - Unable to Write to STDOUT\n");
    }
#endif

    // Setting Up Signal Ignores in Parent:
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    // REPL - looping indefinitely until exited
    // Reads the input and parses it
    while ((count = read(0, buf, BUFF_SIZE)) != 0) {
        // When the -DPROMPT flag is included:
        if (count == -1) {
            // An Error Occured in Reading
            fprintf(stderr, "ERROR - Could not read the line given\n");
        } else {
            // Executes the Command
            if (parse(buf, tokens, execv) < 0) {
                // This line is run only when an error occurs during the
                // parsing.
                fprintf(stderr, "ERROR - Invalid Command Line for Parsing");
                continue;
            }

            // If the Token isn't empty
            if (tokens[0] != NULL) {
                if (strcmp(tokens[0], "exit") == 0) {
                    cleanup_job_list(j_list);
                    exit(0);  // Exit Command
                } else if (strcmp(tokens[0], "cd") == 0) {
                    cd_command(tokens);  // CD Command
                } else if (strcmp(tokens[0], "ln") == 0) {
                    ln_command(tokens);  // Link Command
                } else if (strcmp(tokens[0], "rm") == 0) {
                    rm_command(tokens);  // Remove Command
                } else if (strcmp(tokens[0], "jobs") == 0) {
                    jobs_command(tokens);
                } else if (strcmp(tokens[0], "fg") == 0) {
                    fg_command(tokens);
                } else if (strcmp(tokens[0], "bg") == 0) {
                    bg_command(tokens);
                } else {
                    if (strcmp(last_string(tokens), "&") == 0) {
                        // Enter Background Process (Don't Wait):
                        bin_command(tokens, execv, 1);
                    } else {
                        // All other commands get redirected to here
                        bin_command(tokens, execv, 0);
                        // wait(NULL);
                    }
                }
            }
        }

        // Waiting for all Background Processes here:
        int wret, wstatus;
        while ((wret = waitpid(-1, &wstatus,
                               WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
            // examine all children who’ve terminated or stopped
            int wjid = get_job_jid(j_list, wret);
            if (WIFEXITED(wstatus)) {
                // terminated normally
                remove_job_pid(j_list, wret);
                if (fprintf(stdout,
                            "[%d] (%d) terminated with exit status %d\n", wjid,
                            wret, WEXITSTATUS(wstatus)) < 0) {
                    fprintf(stderr, "ERROR - Error Writing to Output\n");
                }
            }
            if (WIFSIGNALED(wstatus)) {
                // terminated by a signal
                remove_job_pid(j_list, wret);
                if (fprintf(stdout, "[%d] (%d) terminated by signal %d\n", wjid,
                            wret, WTERMSIG(wstatus)) < 0) {
                    fprintf(stderr, "ERROR - Error Writing to Output\n");
                }
            }
            if (WIFSTOPPED(wstatus)) {
                // stopped
                update_job_pid(j_list, wret, STOPPED);
                if (fprintf(stdout, "[%d] (%d) suspended by signal %d\n", wjid,
                            wret, WSTOPSIG(wstatus)) < 0) {
                    fprintf(stderr, "ERROR - Error Writing to Output\n");
                }
            }
            if (WIFCONTINUED(wstatus)) {
                update_job_pid(j_list, wret, RUNNING);
                if (fprintf(stdout, "[%d] (%d) resumed\n", wjid, wret) < 0) {
                    fprintf(stderr, "ERROR in writing");
                }
            }
        }

#ifdef PROMPT
        // Starts with the header "33sh> "
        if (write(1, header, header_length) < 0) {
            fprintf(stderr, "ERROR - Unable to Write to STDOUT\n");
        }
#endif

        // reset local variables for next iteration of while loop
        memset(buf, '\0', BUFF_SIZE);
        for (size_t i = 0; i < TOKS; i++) {
            tokens[i] = NULL;
        }
        for (size_t i = 0; i < TOKS; i++) {
            execv[i] = NULL;
        }
    }

    cleanup_job_list(j_list);
    return 0;
} 




/**
 * The below is just a driving function that immediately calls into another 
 * function that has some greater strengths/ is able to do more than simply 
 * what is done here. This was done for the sake of organization.
 *
 * Assumes a width of 80 characters for the shell.
 *
 */
int main(int argc, char **argv) {

  //debug = 0;
  //verbose = 0;
  
  //for (int i = 1; i < argc; i++) {
  //  if (!strcmp("-d", argv[i])){
  //    debug=1;
  //  }
  //  if (!strcmp("-v", argv[i])){
  //    verbose=1;
  //  }
  //}

  // init
  //WINDOW* w = initscr();
  //noecho();
  //keypad(w, 1);

  // call REPL
  int ret = wmain();
  //delwin(w);
  //endwin();
  return ret;
}
