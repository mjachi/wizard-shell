#include "prompt.h"
#include "misc.h"
#include <dirent.h>
#include <limits.h>

#define _BSD_SOURCE
#define _XOPEN_SORUCE 700
#define UNUSED(x) (void)(x)
#define MAX_HOST 253
#define MAX_PATH 4096
#define BANG_MAX 64

#define RL_BFS 1024
#define PS_BFS 512
#define TOK_DELIM " \t\n"


extern int errno;
job_list_t *jobs_list;
int jcount;

/**
 * prompt.c 
 *
 * Contains I/O and REPL implementations.
 */

int bin_fg(int argc, char **argv) {
    if (argc != 2) {
        printw("\n\tfg: syntax error -- requires exactly one argument");
        return -1;
    }

    if (argv[1][0] != '%') {
        printw("\n\tfg: job input does not begin with %%\n");
        return -1;
    }

    int jid = atoi(argv[1] + 1);
    pid_t pid = get_job_pid(jobs_list, jid);
    if (pid < 0) {
        printw("\n\tjob not found\n");
        return -1;
    }

    tcsetpgrp(STDIN_FILENO, pid);
    if (kill(-1 * pid, SIGCONT) < 0) {
        printw("\n\t{wsh @ bin_fg} -- ERROR in continuing the job");
        return -1;
    }

    int wret, wstatus;
    if ((wret = waitpid(pid, &wstatus, WUNTRACED)) > 0) {
        if (WIFEXITED(wstatus)) {
            remove_job_jid(jobs_list, jid);
        }
        if (WIFSIGNALED(wstatus)) {
            remove_job_jid(jobs_list, jid);
            if (printw("\n[%d] (%d) terminated by signal %d\n", jcount,
                        wret, WTERMSIG(wstatus)) == ERR) {
                printw("\n\t{wsh @ bin_fg} -- ERROR writing to output\n");
            }
        }
        if (WIFSTOPPED(wstatus)) {
            update_job_jid(jobs_list, jid, STOPPED);
            if (printw("\n\t[%d] (%d) suspended by signal %d\n", jcount,
                        wret, WSTOPSIG(wstatus)) == ERR) {
                printw("\n\t{wsh @ bin_fg} -- ERROR writing to output\n");
            }
        }
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
    return 0;
}

int bin_bg(int argc, char **argv) {
    // Checks that there's at least 1 arguments after the ln string
    if (argc != 2) {
        printw("\n\tbg: syntax error -- requires exactly one argument\n");
        return -1;
    }

    if (argv[1][0] != '%') {
        printw("\n\tbg: job input does not begin with %%\n");
        return -1;
    }

    int jid = atoi(argv[1] + 1);
    pid_t pid = get_job_pid(jobs_list, jid);
    if (pid < 0) {
        printw("\n\tjob not found\n");
        return -1;
    }

    if (kill(-1 * pid, SIGCONT) < 0) {
        printw("\n\tERROR in continuing the job");
        return -1;
    }

  return 0;
}


int bin_jobs(int argc, char **argv) {
    if (argc != 1) {
        printw("\n\tjobs: syntax error -- too many arguments; doesn't take any\n");
        return -1;
    }

    jobs(jobs_list);
    return 0;
}


// Tokenize the buffer as desired.
char **wsh_tokenize(char *line) {
  int bufsize = PS_BFS; 
  int pos = 0;

  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "{wsh @ tokenize} -- error allocating for buffer");
    endwin();
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TOK_DELIM);
  while (token != NULL) {
    tokens[pos] = token;
    pos++;

    if (pos >= bufsize) {
      bufsize += PS_BFS;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "{wsh @ tokenize} -- error allocating for buffer");
        endwin();
        exit(EXIT_FAILURE);
      }
    }


    token = strtok(NULL, TOK_DELIM);
  }
  tokens[pos] = NULL;
  return tokens;
}

// Take tokens and returns argv
char **prep (char **tokens) {
  int bufsize = PS_BFS;
  int pos = 0;

  char **argv = malloc(bufsize * sizeof(char*));
  char *tok;

  if (!tokens) {
    fprintf(stderr, "{wsh @ prep} -- error allocating for buffer");
    exit(EXIT_FAILURE);
  }

  while ((tok = tokens[pos])){
    if (is_not_redirect(tok) && (strcmp(tok,"&")!=0)) {
      argv[pos] = tok;
      if (pos >= bufsize) {
        bufsize += PS_BFS;
        argv = realloc(argv, bufsize * sizeof(char*));
        if (!tokens) {
          fprintf(stderr, "{wsh @ prep} -- error allocating for buffer");
          exit(EXIT_FAILURE);
        }
      }
    }
    pos++;
  }

  argv[pos] = NULL;

  if (argv[0]) { // /bin/echo --> echo
    char *last = strrchr(argv[0], '/');
    if (last) {
      argv[0] = last + 1;
    }
  }
  return argv;
}


// executes a given command per tokens and argv
void execute(char **tokens, char **argv,
                 int is_background) {
    // Initial Variables
    char *input = "\0";
    char *clobber = "\0";  // for ">"
    char *dclobber = "\0";  // for ">>"

    int pipefd[2];
    pipe(pipefd);

    int store_errno = 0;

    // find redirection symbols
    for (int i = 0; tokens[i] != NULL; i++) {
      if (strcmp(tokens[i], "<") == 0) {
        if (strcmp(input, "\0") == 0) {
          input = tokens[i + 1];
            
          if (input == NULL) {
            printw("\n\t{wsh @ execute} -- no input file specified\n");
            return;
          }
        } else {
            printw("\n\t{wsh @ execute} -- syntax error: multiple input files\n");
            return;
        }

      } else if (strcmp(tokens[i], ">") == 0) {
        // handle output redirection:
        // Check if the s_output variable has been changed already:
        if (strcmp(clobber, "\0") == 0) {
            clobber = tokens[i + 1];
            if (clobber == NULL) {
                printw("\n\t{wsh @ execute} -- no output file specified\n");
                return;
                }
            } else {
                // If it is, then we have multiple of the same redirection
                // symbols
                printw("\n\t{wsh @ execute} -- syntax error: multiple output files\n");
                return;
            }

        } else if (strcmp(tokens[i], ">>") == 0) {
            if (strcmp(dclobber, "\0") == 0) {
                dclobber = tokens[i + 1];
                if (dclobber == NULL) {
                    printw("\n\t{wsh @ execute} -- no output file specified\n");
                    return;
                }
            } else {
                printw("\n\t{wsh @ execute} -- syntax error: multiple output files\n");
                return;
            }
        }
    }

    if (strcmp(clobber, "\0") != 0 && strcmp(dclobber, "\0") != 0) {
        printw("\n\t{wsh @ execute}syntax error: multiple output files\n");
        return;
    }

    int is_output_double =
        (strcmp(dclobber, "\0") != 0);
    char *output = (is_output_double) ? dclobber : clobber;

    pid_t pid = 0;

    if (!(pid = fork())) { // child process
        pid = getpid();
        setpgid(pid, pid);
        if (!is_background) {
            tcsetpgrp(STDIN_FILENO, pid);
        }

        close(pipefd[0]);

        dup2(pipefd[1],1);
        dup2(pipefd[1],2);

        close(pipefd[1]);

        // for parent
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCONT, SIG_DFL);

        if (strcmp(input, "\0") != 0) {
            if (close(STDIN_FILENO) < 0) {
                endwin();
                fprintf(stderr, "\n\t{wsh @ execute} -- error closing standard input\n");
                exit(1);
            }

            if (open(input, O_RDONLY) < 0) {
                endwin();
                fprintf(stderr, "\n\t{wsh @ execute} -- error opening input file\n");
                exit(1);
            }
        }

        if (strcmp(output, "\0") != 0) {
            if (close(STDOUT_FILENO) < 0) {
               endwin();
               fprintf(stderr, "\n\t{wsh @ execute} -- error closing standard output\n");
                exit(1);
            }

            if (is_output_double) {
                if (open(output, O_RDWR | O_CREAT | O_APPEND, 00600) < 0) {
                    endwin();
                    fprintf(stderr, "\n\t{wsh @ execute} -- error opening output file\n");
                    exit(1);
                }
            } else {
                if (open(output, O_RDWR | O_CREAT | O_TRUNC, 00600) < 0) {
                    fprintf(stderr, "\n\t{wsh @ execute} -- error opening output file\n");
                    exit(1);
                }
            }
        }

        // Executes the command in the child process
        
        execv(first_nonredirect(tokens, "\0"), argv);

        perror("\n\texecv");

        /* we won’t get here unless execv failed */
        char *error = strerror(errno);
        //endwin();
        printw("\n\texecv: %s",error);
        exit(1);
    }

    if (is_background) {

        add_job(jobs_list, ++jcount, pid, RUNNING, first_nonredirect(tokens, "\0"));
        if (printw("\n[%d] (%d)\n", jcount, pid) == ERR) {
            printw("\n\t{wsh @ execute} -- error writing to output\n");
        }

    } else {
        int wret, wstatus;
        if ((wret = waitpid(pid, &wstatus, WUNTRACED)) > 0) {
            if (WIFEXITED(wstatus)) {
            }
            if (WIFSIGNALED(wstatus)) {
                // terminated by a signal
                jcount = jcount + 1;
                if (printw("\n[%d] (%d) terminated by signal %d\n",
                            jcount, wret, WTERMSIG(wstatus)) == ERR) {
                    printw("\n\t{wsh @ execute} -- error writing to output\n");
                }
            }
            if (WIFSTOPPED(wstatus)) {
                add_job(jobs_list, ++jcount, pid, STOPPED, first_nonredirect(tokens, "\0"));
                if (printw("\n[%d] (%d) suspended by signal %d\n",
                            jcount, wret, WSTOPSIG(wstatus)) == ERR) {
                    printw("\n\t{wsh @ execute} -- error writing to output\n");
                }
            }
        }

        // Giving the terminal control back to the Shell
        tcsetpgrp(STDIN_FILENO, getpgrp());

        if (store_errno != 0) {
          char *error = strerror(errno);
          printw("\n\t {wsh @ execv} -- %s", error);
        } else {
          char buffer[2048];
          close(pipefd[1]);
          while (read(pipefd[0], buffer, sizeof(buffer))!=0) {
            printw("\n %s", buffer);
          }
        }


    }
} 
/* sigint_handler
 * Respond to SIGINT signal (CTRL-C)
 *
 * Argument: int sig - the integer code representing this signal
 */
void sigint_handler(int sig) {
  write(STDOUT_FILENO, "\nread a SIGINT\n", 15);
  return;
}

/* sigtstp_handler
 * Respond to SIGTSTP signal (CTRL-Z)
 *
 * Argument: int sig - the integer code representing this signal
 */
void sigtstp_handler(int sig) {
  write(STDOUT_FILENO, "\nread a SIGTSTP\n", 16);
  exit(0);
  return;
}

/* sigquit_handler
 * Catches SIGQUIT signal (CTRL-\)
 *
 * Argument: int sig - the integer code representing this signal
 */
void sigquit_handler(int sig) {
  write(STDOUT_FILENO, "\nread a SIGQUIT\n", 16);
  return;
}

/**
 * splash
 *
 * just a "pretty"-fying function that prints a splash screen with some info.
 */
int splash() {
  int ret = 0;
  FILE *mural_ptr = fopen("../mural.txt", "r");
  if (!mural_ptr) {
    printw("Skipping splash; failed to find mural.txt");
    ret = -1;
  }

  int c;
  while ((c = getc(mural_ptr)) != EOF) {
    putchar(c);
  }
  fclose(mural_ptr);
  printw("wsh (:wizard: shell) \n\n\n");

  refresh();
  
  return ret;
}


/**
 * Main REPL driving function.
 */
int wsh_main(int argc, char **argv) {
  // Setting up the Initial Variables for the Main Function
  char *tokens[TOKS];
  char *execv[TOKS];
  ssize_t count;

  // splash screen
  // splash(); 
  
  WINDOW* w = initscr();
  noecho();
  cbreak();
  keypad(stdscr, TRUE);
  refresh();

  // jobs init
  jobs_list = init_job_list();
  jcount = 0;

  // Setting Up Signal Ignores in Parent:
  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  // set up completions tree

  TrieNode *root = tn_getNode();
  tn_insert(root, "cd");
  tn_insert(root, "ln");
  tn_insert(root, "bg");
  tn_insert(root, "rm");
  tn_insert(root, "jobs");
  tn_insert(root, "fg");
  tn_insert(root, "testforcompletions");
  tn_insert(root, "foobazbarbar");

  // Init the history data structures
  
  history* h = malloc(sizeof(history));
  h->first = NULL;
  h->curr = NULL;
  h->count = 0;

  // init alias HT

  HashTable *aliass = newHashTable(50);

  // environment init
  char *home = getenv("HOME");
  char *uid = getenv("USER");

  char bang[80];
  char cwd[20];
  strcpy(bang, "\n");
  strcat(bang, uid);
  strcat(bang, " @ wsh %% ");
 

  // REPL begins below

  for (;;) { // beginning of this = new command
    
    // reset everything
    refresh();
    int cx;
    int cy;

    if (printw(bang) == ERR) {
        endwin();
        fprintf(stderr, "{wsh @ REPL} -- unable to write to screen\n");
        return -1;
    }

    h->curr = h->first;

    int bfs = RL_BFS;
    int pos = 0;
    // dynamically allocated buffer for longer lines/ paths
    char *buffer = malloc(sizeof(char) * bfs);
    int in;

    // cwd files
    //DIR *d
    //struct dirent *dir;
    //d = opendir()

    if (!buffer) {
      endwin();
      fprintf(stderr, "{wsh @ REPL} -- error allocating for buffer");
      exit(EXIT_FAILURE);
    }

    for (;;) { // beginning of this = taking in another character
      in = getch();
      getsyx(cy,cx);
      if (in == EOF || in == '\n') {
        buffer[pos] = '\0';
        h_push(h, buffer);
        refresh();
        break;
      }

      char *hist_comm;
      int t;

      switch (in) {
        case EOF:
        case '\n':
          buffer[pos] = '\0';
          refresh();
          break;
        case 127:
        case KEY_BACKSPACE:
        case KEY_DC:
          if (pos > 0){ // busted if you try to delete in the middle of the string
            getsyx(cy, cx);
            move(cy, --cx);
            delch();
            buffer[--pos] = '\0';
            refresh();
          }
          break;

          // TODO -- PATH recognition, alias

        case KEY_UP:
          if(pos != 0 || buffer[0]) break;
          if(!h->curr) break;
          int to_break = 1;
          int first_iter = 1;

          while (to_break) {
            t = first_iter ? KEY_UP : getch();
            
            getsyx(cy,cx);
            switch (t) {
              case EOF:
              case '\n':
                to_break=0;
                break;
              case KEY_UP:
                // if there's already a command there
                while (pos > 0) {
                  move(cy,--cx);
                  delch();
                  buffer[pos] = '\0';
                  pos--;
                } // delete key behavior until buffer is clear
                refresh();
                // cursor now at the bang in theory

                // history commands
                // - Move curr to next if there is.
                if (!first_iter && h->curr->next) { 
                  h->curr = h->curr->next;
                }
                // - Get current history step
                hist_comm = h->curr->command;

                // copy hist comm into buffer
                strcpy(buffer, hist_comm);
                // adjust position appropriately
                pos = strlen(hist_comm);

                // print as desired
                printw("%s", buffer);
                refresh();

                break; // end of KEYUP for dialogue
              
              case KEY_DOWN:
                // if there's already a command there
                while (pos > 0) {
                  move(cy,--cx);
                  delch();
                  buffer[pos] = '\0';
                  pos--;
                } // delete key behavior until buffer is clear
                refresh();
                // cursor now at the bang in theory

                // history commands
                // - Move curr to if there is.
                if (h->curr->prev){
                  h->curr = h->curr->prev;
                }
                // - Get current history step
                hist_comm = h->curr->command;

                // copy hist comm into buffer
                strcpy(buffer, hist_comm);
                // adjust position appropriately
                pos = strlen(hist_comm);

                // print as desired
                printw("%s", buffer);
                refresh();
                break; // end of KEYDOWN for dialogue
                
            } // end of switch for dialogue
            first_iter = 0;
          } // end of while for dialogue
          break; // end of KEYUP for REPL

        case KEY_DOWN:
          break;
        
        case KEY_LEFT:
          if (pos >= 0) {
            getsyx(cy,cx);
            move(cy,--cx);
            refresh();
          }
          break;
        
        case KEY_RIGHT:
          getsyx(cy,cx);
          if (cx < pos){
            move(cy,++cx);
          }
          refresh();
          break;
        
        case KEY_STAB: // currently only a single word.
        case '\t':
          if(pos == 0) {
            break;
          }
          refresh();
          getsyx(cy,cx);
          char *temp = strrchr(buffer,' ');
          char *prefix = temp-1 ? buffer : temp;
          char *sugg = completionOf(root, prefix);
          while (pos > 0) {
            move(cy,--cx);
            delch();
            pos--;
          } // delete key behavior until buffer is clear

          printw("%s",sugg);
          pos += strlen(sugg);
          //printw("\n\n%d\n\n", pos);

          refresh();
          
          break;
        
        default:
          addch(in);
          buffer[pos] = in;
          pos++;
      }

      if (pos >= bfs) {
        bfs+=RL_BFS;
        buffer = realloc(buffer, bfs);
        if (!buffer) {
          fprintf(stderr, "\n{wsh @ REPL} -- error allocating for buffer");
          continue;
        }
      }
    }
    refresh();

    // tokenize 
    char **tokens = wsh_tokenize(buffer);
    char **argv = prep(tokens);
    
    //if (tokens[0]) tn_insert(root, tokens[0]);

    if (!tokens){
      // if error while parsing
      fprintf(stderr, "{wsh @ REPL -- parsing} -- ran into generic error parsing\n");
      continue;
    }
    if (!argv) {
      fprintf(stderr, "{wsh @ REPL -- prepping} -- ran into generic error prepping\n");
      continue;
    }
    // since defined, gives the length of each to be passed into everything.
    int tokct = ppstrlen(tokens);
    int argc = ppstrlen(argv);

    // If the Token isn't empty
    if (tokens[0] != NULL) {
      if (strcmp(tokens[0], "exit") == 0) {
        int ec = 0;
        if (tokct == 2){
          ec = atoi(tokens[1]);
        } else if (tokct > 2) {
          printw("\n\texit: syntax error -- too many arguments");
          continue;
        }
        cleanup_job_list(jobs_list);
        endwin();
        exit(ec);  // Exit Command
      } else if (strcmp(tokens[0], "clear") == 0) {
        if (tokct > 1) {
          printw("\n\tclear: syntax error -- doesn't take any arguments");
        }
        clear();
      } else if (strcmp(tokens[0], "cd") == 0) {
        bin_cd(tokct, tokens);   // CD
      } else if (strcmp(tokens[0], "ln") == 0) {
        bin_ln(tokct, tokens);   // Link
      } else if (strcmp(tokens[0], "rm") == 0) {
        bin_rm(tokct, tokens);   // Remove
      } else if (strcmp(tokens[0], "jobs") == 0) {
        bin_jobs(tokct, tokens); // Jobs
      } else if (strcmp(tokens[0], "fg") == 0) {
        bin_fg(tokct, tokens);   // fg
      } else if (strcmp(tokens[0], "bg") == 0) {
        bin_bg(tokct, tokens);   // bg
      } else {
        if (strcmp(ppstr_final(tokens), "&") == 0) {
          execute(tokens, argv, 1);
        } else {
          execute(tokens, argv, 0);
        }
      }
    }

    // Waiting for all Background Processes here:
    int wret, wstatus;
    while ((wret = waitpid(-1, &wstatus,
                         WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
      // examine all children who’ve terminated or stopped
      int wjid = get_job_jid(jobs_list, wret);
      if (WIFEXITED(wstatus)) {
        // terminated normally
        remove_job_pid(jobs_list, wret);
        if (printw( "\n[%d] (%d) terminated with exit status %d\n", wjid,
                    wret, WEXITSTATUS(wstatus)) < 0) {
            endwin();
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
            exit(-1);
          }
        }
        if (WIFSIGNALED(wstatus)) {
          // terminated by signal
          remove_job_pid(jobs_list, wret);
          if (printw("\n[%d] (%d) terminated by signal %d\n", wjid,
                  wret, WTERMSIG(wstatus)) < 0) {
            endwin();
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
            exit(-1);

            }
        }
        if (WIFSTOPPED(wstatus)) {
          // stopped
          update_job_pid(jobs_list, wret, STOPPED);
          if (printw("\n[%d] (%d) suspended by signal %d\n", wjid,
                        wret, WSTOPSIG(wstatus)) < 0) {
           
            endwin();
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
            exit(-1);
          }
        }
        if (WIFCONTINUED(wstatus)) {
              
          update_job_pid(jobs_list, wret, RUNNING);
          if (printw("\n[%d] (%d) resumed\n", wjid, wret) < 0) {
            endwin();
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
            exit(-1);
          }
        }
      }

  }

  cleanup_job_list(jobs_list);
  endwin();
  return 0;
} 

