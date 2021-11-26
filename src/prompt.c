#include "prompt.h"
#include "misc.h"
#include "writeout.h"
#include <dirent.h>
#include <limits.h>
#include <sys/utsname.h>

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
            if (!printf("\n[%d] (%d) terminated by signal %d\n", jcount,
                        wret, WTERMSIG(wstatus))) {
                fprintf(stderr,"\n\t{wsh @ bin_fg} -- error writing to output\n");
            }
        }
        if (WIFSTOPPED(wstatus)) {
            update_job_jid(jobs_list, jid, STOPPED);
            if (!printf("\n\t[%d] (%d) suspended by signal %d\n", jcount,
                        wret, WSTOPSIG(wstatus))) {
                fprintf(stderr,"\n\t{wsh @ bin_fg} -- error writing to output\n");
            }
        }
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());
    return 0;
}

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


int bin_jobs(int argc, char **argv) {
    if (argc != 1) {
        printf("\n\tjobs: syntax error -- too many arguments; doesn't take any\n");
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

    int store_errno = 0;

    // find redirection symbols
    for (int i = 0; tokens[i] != NULL; i++) {
      if (strcmp(tokens[i], "<") == 0) {
        if (strcmp(input, "\0") == 0) {
          input = tokens[i + 1];
            
          if (input == NULL) {
            printf("\n\t{wsh @ execute} -- no input file specified\n");
            return;
          }
        } else {
            printf("\n\t{wsh @ execute} -- syntax error: multiple input files\n");
            return;
        }

      } else if (strcmp(tokens[i], ">") == 0) {
        // handle output redirection:
        // Check if the s_output variable has been changed already:
        if (strcmp(clobber, "\0") == 0) {
            clobber = tokens[i + 1];
            if (clobber == NULL) {
                printf("\n\t{wsh @ execute} -- no output file specified\n");
                return;
                }
            } else {
                // If it is, then we have multiple of the same redirection
                // symbols
                printf("\n\t{wsh @ execute} -- syntax error: multiple output files\n");
                return;
            }

        } else if (strcmp(tokens[i], ">>") == 0) {
            if (strcmp(dclobber, "\0") == 0) {
                dclobber = tokens[i + 1];
                if (dclobber == NULL) {
                    printf("\n\t{wsh @ execute} -- no output file specified\n");
                    return;
                }
            } else {
                printf("\n\t{wsh @ execute} -- syntax error: multiple output files\n");
                return;
            }
        }
    }

    if (strcmp(clobber, "\0") != 0 && strcmp(dclobber, "\0") != 0) {
        printf("\n\t{wsh @ execute}syntax error: multiple output files\n");
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

        // for parent
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCONT, SIG_DFL);

        if (strcmp(input, "\0") != 0) {
            if (close(STDIN_FILENO) < 0) {
                fprintf(stderr, "\n\t{wsh @ execute} -- error closing standard input\n");
                exit(1);
            }

            if (open(input, O_RDONLY) < 0) {
                fprintf(stderr, "\n\t{wsh @ execute} -- error opening input file\n");
                exit(1);
            }
        }

        if (strcmp(output, "\0") != 0) {
            if (close(STDOUT_FILENO) < 0) {
               fprintf(stderr, "\n\t{wsh @ execute} -- error closing standard output\n");
                exit(1);
            }

            if (is_output_double) {
                if (open(output, O_RDWR | O_CREAT | O_APPEND, 00600) < 0) {
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
        
        execvp(first_nonredirect(tokens, "\0"), argv);

        perror("\n\n\t{wsh @ execvp}");
        exit(1);
    }

    if (is_background) {

        add_job(jobs_list, ++jcount, pid, RUNNING, first_nonredirect(tokens, "\0"));
        if (!printf("\n[%d] (%d)\n", jcount, pid)) {
            fprintf(stderr,"\n\t{wsh @ execute} -- error writing to output\n");
        }

    } else {
        int wret, wstatus;
        if ((wret = waitpid(pid, &wstatus, WUNTRACED)) > 0) {
            if (WIFEXITED(wstatus)) {
            }
            if (WIFSIGNALED(wstatus)) {
                // terminated by a signal
                jcount = jcount + 1;
                if (!printf("\n[%d] (%d) terminated by signal %d\n",
                            jcount, wret, WTERMSIG(wstatus))) {
                    fprintf(stderr,"\n\t{wsh @ execute} -- error writing to output\n");
                }
            }
            if (WIFSTOPPED(wstatus)) {
                add_job(jobs_list, ++jcount, pid, STOPPED, first_nonredirect(tokens, "\0"));
                if (!printf("\n[%d] (%d) suspended by signal %d\n",
                            jcount, wret, WSTOPSIG(wstatus))) {
                    fprintf(stderr,"\n\t{wsh @ execute} -- error writing to output\n");
                }
            }
        }

        // Giving the terminal control back to the Shell
        tcsetpgrp(STDIN_FILENO, getpgrp());

        if (store_errno != 0) {
          char *error = strerror(errno);
          printf("\n\t {wsh @ execv} -- %s", error);
        }
    }
} 

/**
 * splash
 *
 * just a "pretty"-fying function that prints a splash screen with some info.
 */
int splash() {
  FILE *mural_ptr = fopen("mural.txt", "r");
  if (mural_ptr == NULL) {
    printf("Skipping splash; failed to find mural.txt");
    return -1;
  }

  int c;
  while ((c = getc(mural_ptr)) != EOF) {
    putchar(c);
  }
  fclose(mural_ptr);
  

  struct utsname unameData;
  int ret = uname(&unameData);
  
  if (ret) {
    printf("wsh (:wizard: shell)");
    return ret;
  }

  printf("wsh (:wizard: shell) on %s running %s, \n\n", unameData.nodename, unameData.release);
  
  return 0;
}


// get_line
//
// meant to abstract away the line reading behavior.
// After this, we resolve alias's. Requires access to history
// and completions tree.
char *get_line(history* hist, TrieNode *completions) {
  int bufsize = RL_BFS;
  int pos = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  char c;
  hist->curr = NULL;
  int hist_iter = 0;

  if (!buffer) {
    fprintf(stderr, "{wsh @ get_line} -- buffer allocation error\n");
    exit(EXIT_FAILURE);
  }

  for (;;) {
    c = getch();

    // This would be better handled with a switch statement...
    // but if-else-if-else works fine.
    // TODO -- switch statement instead

    if (c == EOF || c == '\n') { // when user hits enter
      buffer[pos] = '\0';

      if (strlen(buffer) > 0) {
        /**
         * We only want to consider adding nonempty strings to the history
         * LL. Then, we also don't want to add repeats (ie, if the current
         * buffer matches the most recent command don't add it); this 
         * presents some obvious issues w.r.t. seg-faulting if we try 
         * to check the first command when the history is empty, hence
         * this grossly nested block of conditionals
         */

        if (hist->count > 0 && strcmp(buffer, hist->first->command)) {
          h_push(hist,buffer);
        } else if (hist->count == 0) {
          h_push(hist, buffer);
        }
      }
      return buffer;
    } else if (c == 27 && getch() == 91) { // arrow keys
      switch (getch()){ // Platform specific
        case 65: // Up
          if (!hist->first) {
            break;
          } else if (hist_iter == 0 && hist->first) {
            // Clear buffer && line
            clear_line_buffer(strlen(buffer),pos);
            memset(buffer, 0, sizeof(buffer));
            // Copy command into buffer
            char *hcm = hist->first->command;
            strcpy(buffer, hcm);
            // Set frontend accordingly
            printf("%s",hcm);
            pos = strlen(hcm);
            // Increment
            hist->curr = hist->first;
            hist_iter = 1;
          } else if (hist->curr->next) {
            // Clear buffer && line
            clear_line_buffer(strlen(buffer), pos);
            memset(buffer, 0, sizeof(buffer));
            // Copy command into buffer
            char *hcm = hist->curr->next->command;
            strcpy(buffer, hcm);
            // Set frontend accordingly
            printf("%s", hcm);
            pos = strlen(hcm);
            // Increment
            hist->curr = hist->curr->next;
          }
          // Any other cases should be left as is.
          break;
        case 66: // Down
          if (hist_iter == 0 || !hist->first) {
            break;
          } else if (hist->curr->prev) {
            // Clear buffer && line
            clear_line_buffer(strlen(buffer), pos);
            memset(buffer, 0, sizeof(buffer));
            // Copy command into buffer
            char *hcm = hist->curr->prev->command;
            strcpy(buffer, hcm);
            // Set frontend accordingly
            printf("%s",hcm);
            pos = strlen(hcm);
            // Decrement
            hist->curr = hist->curr->prev;
          } else if (!hist->curr->prev && strlen(buffer) > 0) {
            // Clear buffer && line
            clear_line_buffer(strlen(buffer), pos);
            memset(buffer, 0, sizeof(buffer));
            pos = 0;
          }
          // Any other cases should be left as is.
          break;
        case 67: // Right
          if (pos < strlen(buffer)) {
            putchar(buffer[pos++]);
          }
          break;
        case 68: //Left
          if (pos > 0) {
            putchar('\b');
            pos--;
          }
          break;
      }
      continue;
    } else if (c == '\t' || c == 9) {
      // TAB for completion... if not particular, inputs \t
      if (buffer[pos-1] == ' ' || strlen(buffer) == 0 || buffer[pos-1] == '\t') {
        // in this case, we don't have anything to put in the
        // buffer --- we just want to place 4 spaces
        //
        // n.b. using the escaped tab character can cause issues
        // and spaces > tabs regardless.
        for (int i = 0; i < 4; i++) {
            buffer[pos++] = ' ';
            putchar(' ');
        }
      } else { // want to try to complete the last token now
        // Get the last token.
        char *f_tok = strrchr(buffer, ' ');
        if (!(f_tok)) { 
          // if null, then there are no spaces
          f_tok = buffer;
        } else {
          // if we reach this case, then we simply need to
          // increment the pointer by one to capture the last 
          // word.
          f_tok = f_tok + 1;
        }

        char prefix[strlen(f_tok)];
        strcpy(prefix, f_tok);
        // to ensure that we don't run into pointer errors 
        // with this.

        // Finds the completion of
        char *finish = completionOf(completions, prefix);
        //clear_line_buffer(sizeof(buffer), pos);
        int b_len = strlen(f_tok);
        int final_char = strlen(buffer) - 1;
        for (int i = 0; i < b_len; i++) {
            buffer[final_char-i] = '\0';
        }
        clear_line_buffer(sizeof(buffer), pos);
        strcat(buffer, finish);
        pos = strlen(buffer);
        printf("%s", buffer);

        if (pos >= bufsize) {
          bufsize += RL_BFS;
          buffer = realloc(buffer, bufsize);
          if (!buffer) {
            fprintf(stderr, "{wsh @ get_line} -- could not resize buffer\n");
            exit(EXIT_FAILURE);
          }
        }

      }
      continue;
    } else if (c == 127) {
      // Backspace character
      //
      // Nested cond since we only want to do this
      // if pos > 0; if not, then ignore the backspace
      // and take in the next character
      if (pos > 0) {
          // Easiest to clear
          clear_line_buffer(strlen(buffer), pos);
          // Remove the last character
          pos--;
          buffer[pos] = '\0';
          // Reset stdout
          printf("%s", buffer);
          continue;
      }
    } else {
      buffer[pos++] = c;
      putchar(c);
    }


    if (fflush(stdout) < 0) {
      fprintf(stderr, "{wsh @ get_line} -- could not flush stdout");
      exit(EXIT_FAILURE);
    }

    if (pos >= bufsize) {
      bufsize += RL_BFS;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "{wsh @ get_line} -- could not resize buffer\n");
        exit(EXIT_FAILURE);
      }
    }
  }
  return NULL;
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
  //splash(); 
  
  // jobs init

  jobs_list = init_job_list();
  jcount = 0;

  // Setting up signal ignores in parent:

  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  // set up completions tree

  TrieNode *completions = tn_getNode();

  // add builtins
  tn_insert(completions, "cd");
  tn_insert(completions, "ln");
  tn_insert(completions, "bg");
  tn_insert(completions, "rm");
  tn_insert(completions, "jobs");
  tn_insert(completions, "fg");

  // Grab $PATH from env
  char *pathvar = getenv("PATH");

  if (pathvar) {
    char *path;
    int i;

    // tokenize on colon to get paths
    // then use that immediately to 
    // scandir, and add everything in 
    // there to the completions system
    path = strtok(pathvar, ":");
    while (path) {
      // Scan directory
      struct dirent **fListTemp;
      int num_files = scandir(path, &fListTemp, NULL, alphasort);
      // For the number of files in there,
      // want to add only if curr is entirely
      // made of letters (e.g. g++ and some of the
      // NVIDIA/ CUDA things will break this, among
      // others with e.g. dashes and so on)
      for (i = 0; i < num_files; i++) {
        char *curr = fListTemp[i]->d_name;
        if (strcmp(curr, ".")==0 || strcmp(curr, "..")==0){
          continue;
        } else if (notalpha(curr)) {
          continue;
        } else {
          str_tolower(curr);
          tn_insert(completions, curr);
        }
      }
      for (i = 0; i < num_files; i++) {
        free(fListTemp[i]);
      }
      free (fListTemp);
      path = strtok(NULL, ":");
    }
  } else {
    fprintf(stderr, "{wsh @ init} -- $PATH variable could not be found?");
  }

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

  if (!uid) {
    uid = "user";
  }

  if (!home) {
    home = "/";
  }

  char bang[80];
  char cwd[20];
  strcpy(bang, "\n");
  strcat(bang, uid);
  strcat(bang, " @ wsh % ");

  // Start in user's home directory
  
  if (chdir(home) < 0) {
    perror("{wsh @ init chdir}");
  }

  // REPL begins below

  for (;;) { // beginning of this = new command
    
    if (printf("%s",bang) < 0) { // scuffed try catch for when we reach the bottom of the screen.
      fprintf(stderr, "{wsh @ REPL} -- unable to write to screen\n");
      return -1;
    }

    if (fflush(stdout) < 0) {
      fprintf(stderr, "{wsh @ REPL} -- unable to flush stdout\n");
      return -1;
    }

    h->curr = h->first;

    char *buffer = get_line(h, completions);

    // tokenize 
    char **tokens = wsh_tokenize(buffer);
    char **argv = prep(tokens);
    
    //if (tokens[0]) tn_insert(completions, tokens[0]);

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

    // TODO -- builtin hashtable
    if (tokct > 0) {
      if (strcmp(tokens[0], "exit") == 0) {
        int ec = 0;
        if (tokct == 2){
          ec = atoi(tokens[1]);
        } else if (tokct > 2) {
          printf("\n\texit: syntax error -- too many arguments");
          continue;
        }
        cleanup_job_list(jobs_list);
        exit(ec);  // Exit Command
      } else if (strcmp(tokens[0], "clear") == 0) {
        if (tokct > 1) {
          printf("\n\tclear: syntax error -- doesn't take any arguments");
        }
        if (printf("\e[1;1H\e[2J")) {
          fprintf(stderr, "{wsh @ clear} -- ANSI clear string failed to print");
        }
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
        if (printf( "\n[%d] (%d) terminated with exit status %d\n", wjid,
                    wret, WEXITSTATUS(wstatus)) < 0) {
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
            exit(-1);
          }
        }
        if (WIFSIGNALED(wstatus)) {
          // terminated by signal
          remove_job_pid(jobs_list, wret);
          if (printf("\n[%d] (%d) terminated by signal %d\n", wjid,
                  wret, WTERMSIG(wstatus)) < 0) {
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
            exit(-1);

            }
        }
        if (WIFSTOPPED(wstatus)) {
          // stopped
          update_job_pid(jobs_list, wret, STOPPED);
          if (printf("\n[%d] (%d) suspended by signal %d\n", wjid,
                        wret, WSTOPSIG(wstatus)) < 0) {
           
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
            exit(-1);
          }
        }
        if (WIFCONTINUED(wstatus)) {
              
          update_job_pid(jobs_list, wret, RUNNING);
          if (printf("\n[%d] (%d) resumed\n", wjid, wret) < 0) {
            fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
            exit(-1);
          }
        }
      }

  }

  cleanup_job_list(jobs_list);
  return 0;
} 

