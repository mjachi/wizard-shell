#include "prompt.h"
#include "misc.h"
#include "ncurses.h"

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
        fprintf(stderr, "fg: \n");
        return -1;
    }

    if (argv[1][0] != '%') {
        fprintf(stderr, "fg: job input does not begin with %%\n");
        return -1;
    }

    int jid = atoi(argv[1] + 1);
    pid_t pid = get_job_pid(jobs_list, jid);
    if (pid < 0) {
        fprintf(stderr, "job not found\n");
        return -1;
    }

    tcsetpgrp(STDIN_FILENO, pid);
    if (kill(-1 * pid, SIGCONT) < 0) {
        fprintf(stderr, "ERROR in continuing the job");
        return -1;
    }

    int wret, wstatus;
    if ((wret = waitpid(pid, &wstatus, WUNTRACED)) > 0) {
        if (WIFEXITED(wstatus)) {
            remove_job_jid(jobs_list, jid);
        }
        if (WIFSIGNALED(wstatus)) {
            remove_job_jid(jobs_list, jid);
            if (fprintf(stdout, "[%d] (%d) terminated by signal %d\n", jcount,
                        wret, WTERMSIG(wstatus)) < 0) {
                fprintf(stderr, "ERROR - Error Writing to Output\n");
            }
        }
        if (WIFSTOPPED(wstatus)) {
            update_job_jid(jobs_list, jid, STOPPED);
            if (fprintf(stdout, "[%d] (%d) suspended by signal %d\n", jcount,
                        wret, WSTOPSIG(wstatus)) < 0) {
                fprintf(stderr, "ERROR - Error Writing to Output\n");
            }
        }
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
    return 0;
}

int bin_bg(int argc, char **argv) {
    // Checks that there's at least 1 arguments after the ln string
    if (argc != 2) {
        fprintf(stderr, "bg: syntax error\n");
        return -1;
    }

    if (argv[1][0] != '%') {
        fprintf(stderr, "bg: job input does not begin with %%\n");
        return -1;
    }

    int jid = atoi(argv[1] + 1);
    pid_t pid = get_job_pid(jobs_list, jid);
    if (pid < 0) {
        fprintf(stderr, "job not found\n");
        return -1;
    }

    if (kill(-1 * pid, SIGCONT) < 0) {
        fprintf(stderr, "ERROR in continuing the job");
        return -1;
    }

  return 0;
}


int bin_jobs(int argc, char **argv) {
    if (argc != 1) {
        fprintf(stderr, "jobs: syntax error\n");
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
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TOK_DELIM);
  while (token != NULL) {
    tokens[pos] = token;
    printf("\ntoken %d is %s", pos, token);
    pos++;

    if (pos >= bufsize) {
      bufsize += PS_BFS;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "{wsh @ tokenize} -- error allocating for buffer");
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
    if (!is_redirect(tok) && !strcmp("&",tok)) {
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

  char *bang = "\nwsh > ";

  // Setting Up Signal Ignores in Parent:
  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  // set up completions tree

  TrieNode *root = tn_newNode();

  // Init the history data structures
  
  history* h = malloc(sizeof(history));
  h->first = NULL;
  h->curr = NULL;
  h->count = 0;

  // init builtins HT

  HashTable *builtins = newHashTable(25);
    // add the builtins

  // init alias HT

  HashTable *aliass = newHashTable(50);

  // environment init
  char *home = getenv("HOME");
  char *uid = getenv("USER");
 

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


    // reinitialize the completions database
    
      // add commands from the path.

      // then aliases

      // then files from CWD


    if (!buffer) {
      endwin();
      fprintf(stderr, "{wsh @ REPL} -- error allocating for buffer");
      exit(EXIT_FAILURE);
    }

    int keyct = 0;

    for (;;) { // beginning of this = taking in another character
      in = getch();
      getsyx(cy,cx);
      if (in == EOF || in == '\n') {
        buffer[pos] = '\0';
        printw("\n buffer contains %s\n", buffer);
        printw("%d\n", pos);
        printw("%d\n", keyct);
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

          // TODO -- completions, make sure everything tokens
          // ---- and runs,
          //   proper bang, change into home directory,
          //   PATH recognition, alias, shortcuts

        case KEY_UP:
          if(pos != 0 || buffer[0]) break;
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
                printw("%s", hist_comm);
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
        
        case KEY_STAB:
        case '\t':
          break;
        
        default:
          keyct++;
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

    continue;

    // tokenize 
    char **tokens = wsh_tokenize(buffer);
    char **argv = prep(tokens);
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

    continue;

    // If the Token isn't empty
    if (tokens[0] != NULL) {
      if (strcmp(tokens[0], "exit") == 0) {
        cleanup_job_list(jobs_list);
        exit(0);  // Exit Command
      } else if (strcmp(tokens[0], "cd") == 0) {
        bin_cd(tokct, tokens);  // CD Command
      } else if (strcmp(tokens[0], "ln") == 0) {
        bin_ln(tokct, tokens);  // Link Command
      } else if (strcmp(tokens[0], "rm") == 0) {
        bin_rm(tokct, tokens);  // Remove Command
      } else if (strcmp(tokens[0], "jobs") == 0) {
        bin_jobs(tokct, tokens);
      } else if (strcmp(tokens[0], "fg") == 0) {
        bin_fg(tokct, tokens);
      } else if (strcmp(tokens[0], "bg") == 0) {
        bin_bg(tokct, tokens);
      } else {
        if (strcmp(ppstr_final(tokens), "&") == 0) {
          // Enter Background Process (Don't Wait):
          // execute(tokens, execv, 1);
        } else {
          // All other commands get redirected to here
          // execute(tokens, execv, 0);
          // wait(NULL);
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
        if (fprintf(stdout,
              "[%d] (%d) terminated with exit status %d\n", wjid,
                    wret, WEXITSTATUS(wstatus)) < 0) {
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
          }
        }
        if (WIFSIGNALED(wstatus)) {
          // terminated by signal
          remove_job_pid(jobs_list, wret);
          if (fprintf(stdout, "[%d] (%d) terminated by signal %d\n", wjid,
                  wret, WTERMSIG(wstatus)) < 0) {
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
            }
        }
        if (WIFSTOPPED(wstatus)) {
          // stopped
          update_job_pid(jobs_list, wret, STOPPED);
          if (fprintf(stdout, "[%d] (%d) suspended by signal %d\n", wjid,
                        wret, WSTOPSIG(wstatus)) < 0) {
           
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
          }
        }
        if (WIFCONTINUED(wstatus)) {
              
          update_job_pid(jobs_list, wret, RUNNING);
          if (fprintf(stdout, "[%d] (%d) resumed\n", wjid, wret) < 0) {
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
          }
        }
      }

  }

  cleanup_job_list(jobs_list);
  endwin();
  return 0;
} 

