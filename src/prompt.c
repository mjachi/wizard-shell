#include "prompt.h"
#include "global.h"
#include "misc.h"
#include "writeout.h"

#define MAX_HOST 253
#define MAX_PATH 4096
#define BANG_MAX 64

#define RL_BFS 1024
#define PS_BFS 512
#define TOK_DELIM " \t\n"

#define BFS 1024
#define TOKS 512

extern int errno;
int jcount = 0;
job_list_t *jobs_list = NULL;

builtins_table *BI_TABLE = NULL;
alias_table *AL_TABLE = NULL;

// remember to work with *copies* of these

char *HOME_PATH = NULL;
char *PATH_VAR = NULL;

/**
 * prompt.c
 *
 * Contains I/O and REPL implementations.
 */

/**
 * Tokenizes the line as desired.
 *
 * Parameters:
 * - line: just the raw input line (after resolving aliases
 *   and shortcuts)
 *
 * Returns char**, an array with the tokens on ' '
 *
 *
 */

char **wsh_tokenize(char *line) {
  int bufsize = PS_BFS;
  int pos = 0;

  char **tokens = malloc(bufsize * sizeof(char *));
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
      tokens = realloc(tokens, bufsize * sizeof(char *));
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

/**
 * Processes the tokens as desired.
 *
 * Parameters:
 * - tokens: just the output of wsh_tokenize, in short
 *
 * Returns: another char** with a processed set of tokens
 * e.g. /bin/echo -> echo
 * and some others
 *
 */
char **prep(char **tokens) {
  int bufsize = PS_BFS;
  int pos = 0;

  char **argv = malloc(bufsize * sizeof(char *));
  char *tok;

  if (!tokens) {
    fprintf(stderr, "{wsh @ prep} -- error allocating for buffer");
    exit(EXIT_FAILURE);
  }

  while ((tok = tokens[pos])) {
    if (is_not_redirect(tok) && (strcmp(tok, "&") != 0)) {
      argv[pos] = tok;
      if (pos >= bufsize) {
        bufsize += PS_BFS;
        argv = realloc(argv, bufsize * sizeof(char *));
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

/**
 * Processes an input alias definition
 *
 * MUST be of the form
 *
 * > alias name = here is the definition
 *
 * errs otherwise. No space allowed in
 * between the equal sign as with zsh.
 * Will note expect anything after the
 * second quote.
 *
 * Parameters:
 * - argc: the count of argv
 * - argv: raw input tokens
 */

int process_alias(int tok_count, char **tokens, int suppress_output) {
  if (tok_count < 4) {
    fprintf(stderr, "\n\t{wsh @ process_alias} -- needs a definition");
    return -1;
  }

  if (AL_TABLE == NULL) {
    fprintf(stderr, "\n\t{wsh @ process_alias} -- alias table is not defined; \
      this may be indicative of an allocation or overflow that was not caught \
      elsewhere. Exiting");
    exit(EXIT_FAILURE);
  }

  int seen_equals = 0;
  int seen_first_quote = 0;

  char *name = tokens[1];

  if (strcmp(tokens[2], "=") != 0) {
    fprintf(
        stderr,
        "\n\t{wsh @ process_alias} -- bad formatting; = was not found as the 3rd token from your\
      input. Recall \"alias name = here is the definition\"");
    return -1;
  }

  int bufsize = PS_BFS;

  char **tok_def = malloc(sizeof(char *) * bufsize);

  int i = 3;

  while (tokens[i]) {
    tok_def[i - 3] = tokens[i];

    if (i + 1 >= bufsize) {
      bufsize += PS_BFS;
      tok_def = realloc(tok_def, bufsize * sizeof(char *));
      if (!tokens) {
        fprintf(stderr, "{wsh @ prep} -- error allocating for buffer");
        exit(EXIT_FAILURE);
      }
    }
    i++;
  }

  if (at_set(AL_TABLE, name, tok_def) != 0) {
    fprintf(stderr, "\n\t{wsh @ process_alias} -- failed to add the given "
                    "definition to the alias table");
    return -1;
  }

  if (!suppress_output) {
    printf("\nAdded alias %s with definition: \n", name);
    print_arr(tok_def);
  }
  return 0;
}

/**
 * Searches for a .wshrc in one a few places
 *
 * If it exists, we set values accordingly and place aliases
 * in the table, surpressing output.
 *
 * Of course, if it doesn't exist, we simply return and print a message.
 */

int wsh_rc_init() {

  printf("here");

  size_t home_len = strlen(HOME_PATH);
  char hpc[home_len];
  strcpy(hpc, HOME_PATH);
  char config_path[home_len];

  printf("asdfasdf");

  FILE *config_file = NULL;

  printf("%s", hpc);

  strcat(hpc, ".wshrc");

  if (access(".wshrc", R_OK)) { // Look in cwd
    config_file = fopen(".wshrc", "r");
  } else if (access(hpc, R_OK)) { // Look in ~
    config_file = fopen(hpc, "r");
  } else if (access(config_path, R_OK)) { // Look in ~/.config
    config_file = fopen(config_path, "r");
  } else if (access("/.wshrc", R_OK)) { // Look in /
    config_file = fopen("/.wshrc", "r");
  } else {
    return -1;
  }

  ssize_t length;
  char *buf = NULL;
  size_t buf_len = 128;

  while ((length = getline(&buf, &buf_len, config_file)) >= 0) {
    char **tkns = wsh_tokenize(buf);
    int tokct = ppstrlen(tkns);

    if (!tkns || tokct < 3) {
      fprintf(stderr, "{wsh @ rc init} -- failed to tokenize properly");
      return -1;
    }

    if (strcmp(tkns[0], "alias")) {
      process_alias(tokct, tkns, 0);
    } else if (strcmp(tkns[0], "home")) {
      DIR *dir = opendir(tkns[2]);
      if (dir) {
        HOME_PATH = tkns[2];
      } else {
        fprintf(
            stderr,
            "{wsh @ rc init} -- failed to find the proposed HOME path reset");
      }
    }
  }

  return 0;
}

/**
 * Returns a new set of tokens with aliases filled in. Also
 * fills in the following harcoded shortcuts:
 *
 * "*" -- when a standalone token, extends into all files
 * and folders excluding the links to cwd and pwd.
 *
 * Currently, won't play nicely with e.g. "*.c", which
 * the user likely intended to fill out with everything
 * in cwd that had the extension for a C source file
 *
 * "~" -- as usual, routes to the current user's home directory
 * if it can be found, otherwise errs and re-enters the REPL.
 *
 * These are hardcoded, but this could definitely be done
 * with something similar to the alias hashtables
 *
 * Parameters:
 * - tokens: the raw input tokens.
 * - aliass: pointer to the hash_table that contains all the
 *   aliass for a run time.
 *
 * Returns: an array of strings where each token is replaced
 * with a value in the alias HT if there is a match.
 * e.g. "l /etc" --> "ls -la /etc", granted that there is a
 * match in HT for "l" --> "ls -la", as is typical.
 */

char **resolve_alias_shortcuts(char **tokens) {

  if (!AL_TABLE) {
    fprintf(
        stderr,
        "{wsh @ resolve_alias} -- alias table is a null pointer. This may be indicative\
      of overflow/ allocation issues elsewhere, or it simply wasn't initialized. Exiting.");
    exit(EXIT_FAILURE);
  }

  /**
   * bufsize -- manages the size of f_tokens
   *
   * i -- used to index tokens (the input)
   *
   * pos -- used to index f_tokens
   *
   * j -- used to index whatever we need to
   *      in the given context
   */
  int bufsize = PS_BFS;
  int i = 0;
  int pos = 0;

  char **f_tokens = malloc(bufsize * sizeof(char *));

  if (!f_tokens) {
    fprintf(stderr,
            "{wsh @ tokenize} -- error allocating for resolved token array");
    exit(EXIT_FAILURE);
  }

  while (tokens[i] != NULL) {

    char **alias_toks;
    if (strcmp(tokens[i], "~") == 0) { // ~ shortcut
      if (pos + 1 >= bufsize) {
        bufsize += PS_BFS;
        f_tokens = realloc(f_tokens, bufsize * sizeof(char *));
        if (!f_tokens) {
          fprintf(stderr, "{wsh @ alias's and shortcuts} -- error reallocating "
                          "for resolved token array");
          exit(EXIT_FAILURE);
        }
      }
      char *home = strcpy(home, HOME_PATH);
      f_tokens[pos++] = home;
    } else if (strcmp(tokens[i], "*") == 0) { // * shortcut
      char cwd_path[PATH_MAX];
      cwd_path[0] = 0;

      if (getcwd(cwd_path, sizeof(cwd_path)) == NULL) {
        fprintf(stderr, "{wsh @ alias's and shortcuts} -- couldn't retrieve "
                        "the current working directory");
      }

      struct dirent **fListTemp;
      int num_files = scandir(cwd_path, &fListTemp, NULL, alphasort);

      if (pos + num_files >= bufsize) {
        bufsize += PS_BFS;
        f_tokens = realloc(f_tokens, bufsize * sizeof(char *));
        if (!f_tokens) {
          fprintf(stderr, "{wsh @ alias's and shortcuts} -- error reallocating "
                          "for resolved token array");
          exit(EXIT_FAILURE);
        }
      }

      int j;
      for (j = 0; j < num_files; j++) {
        char *curr = fListTemp[j]->d_name;
        int is_hidden = (strlen(curr) > 0) ? curr[0] == '.' : 1;
        if (strcmp(curr, ".") == 0 || strcmp(curr, "..") == 0 || is_hidden) {
          continue;
        }

        f_tokens[pos++] = curr;
        printf("\nAdded %s", f_tokens[pos - 1]);
      }
      int k;
      for (k = 0; k < num_files; k++) {
        free(fListTemp[k]);
      }
      free(fListTemp);

      printf("\n num_files - j = %d", num_files - j);

    } else if ((alias_toks = at_get(AL_TABLE, tokens[i])) != NULL) { // alias's

      int al_len = ppstrlen(alias_toks);
      printf("\nal_len %d", al_len);
      if (pos + al_len >= bufsize) {
        bufsize += PS_BFS;
        tokens = realloc(tokens, bufsize * sizeof(char *));
        if (!tokens) {
          fprintf(stderr, "{wsh @ alias's and shortcuts} -- error reallocating "
                          "for resolved token array");
          exit(EXIT_FAILURE);
        }
      }

      int j;
      for (j = 0; j < al_len; j++) {
        f_tokens[pos++] = alias_toks[j];
      }

    } else {
      f_tokens[pos++] = tokens[i];
    }

    i++;
  }
  f_tokens[pos] = NULL;
  return f_tokens;
}

/**
 * Executes a given command based on the tokens and argv
 * This also contains all of the file redirection
 * and most of the jobs handling logic.
 *
 * Parameters:
 * tokens -- the raw tokenized
 * argv -- the processed tokenized
 * is_background -- 1 if the given command
 *    is meant to be bg'ed, 0 otherwise.
 */
void execute(char **tokens, char **argv, int is_background) {
  char *input = "\0";
  char *clobber = "\0";  // for ">"
  char *dclobber = "\0"; // for ">>"

  int store_errno = 0;

  // Find redirection symbols
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

  int is_output_double = (strcmp(dclobber, "\0") != 0);
  char *output = (is_output_double) ? dclobber : clobber;

  pid_t pid = 0;

  // Fork to reach child process
  if (!(pid = fork())) {
    pid = getpid();
    setpgid(pid, pid);
    if (!is_background) {
      tcsetpgrp(STDIN_FILENO, pid);
    }

    // For parent process.
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCONT, SIG_DFL);

    if (strcmp(input, "\0") != 0) {
      if (close(STDIN_FILENO) < 0) {
        fprintf(stderr,
                "\n\t{wsh @ execute} -- error closing standard input\n");
        exit(1);
      }

      if (open(input, O_RDONLY) < 0) {
        fprintf(stderr, "\n\t{wsh @ execute} -- error opening input file\n");
        exit(1);
      }
    }

    if (strcmp(output, "\0") != 0) {
      if (close(STDOUT_FILENO) < 0) {
        fprintf(stderr,
                "\n\t{wsh @ execute} -- error closing standard output\n");
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

    char *fnr = first_nonredirect(tokens, "\0");
    execvp(fnr, argv);
    perror("\n\t{wsh @ execvp} ");
    exit(1);
  }

  if (is_background) {

    add_job(jobs_list, ++jcount, pid, RUNNING, first_nonredirect(tokens, "\0"));
    if (!printf("[%d] (%d)\n", jcount, pid)) {
      fprintf(stderr, "\n\t{wsh @ execute} -- error writing to output\n");
    }

  } else {
    int wret, wstatus;
    if ((wret = waitpid(pid, &wstatus, WUNTRACED)) > 0) {
      if (WIFEXITED(wstatus)) {
      }
      if (WIFSIGNALED(wstatus)) {
        // terminated by a signal
        jcount = jcount + 1;
        if (!printf("[%d] (%d) terminated by signal %d\n", jcount, wret,
                    WTERMSIG(wstatus))) {
          fprintf(stderr, "\n\t{wsh @ execute} -- error writing to output\n");
        }
      }
      if (WIFSTOPPED(wstatus)) {
        add_job(jobs_list, ++jcount, pid, STOPPED,
                first_nonredirect(tokens, "\0"));
        if (!printf("[%d] (%d) suspended by signal %d\n", jcount, wret,
                    WSTOPSIG(wstatus))) {
          fprintf(stderr, "\n\t{wsh @ execute} -- error writing to output\n");
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
 * just a "pretty"-fying function that prints a splash screen with
 * some info. Splash screen is precisely whatever's found in
 * the mural.txt in the project root directory.
 *
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

  int ret = 0;
  struct utsname unameData;
  if ((ret = uname(&unameData)) != 0) {
    printf("wsh (:wizard: shell)");
    return ret;
  }

  printf("wsh (:wizard: shell) on %s running %s, \n\n", unameData.nodename,
         unameData.release);
  return 0;
}

/**
 * get_line
 *
 * Reads in a line from the user using a custom implementation
 * of getch() to mimic the one functionality from ncurses that
 * is needed for this --- basically just messes with termios
 * such that we read in the character and still have complete
 * and total control over what we do with it. Hence, we must
 * ensure that each arbitrary character is placed into the
 * buffer and printed out
 *
 */
char *get_line(history *hist, TrieNode *completions) {
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
          h_push(hist, buffer);
        } else if (hist->count == 0) {
          h_push(hist, buffer);
        }
      }
      return buffer;
    } else if (c == 27 && getch() == 91) { // arrow keys
      switch (getch()) {                   // Platform specific
      case 65:                             // Up
        if (!hist->first) {
          break;
        } else if (hist_iter == 0 && hist->first) {
          // Clear buffer && line
          clear_line_buffer(strlen(buffer), pos);
          memset(buffer, '\0', bufsize * sizeof(char));
          // Copy command into buffer
          char *hcm = hist->first->command;
          strcpy(buffer, hcm);
          // Set frontend accordingly
          printf("%s", hcm);
          pos = strlen(hcm);
          // Increment
          hist->curr = hist->first;
          hist_iter = 1;
        } else if (hist->curr->next) {
          // Clear buffer && line
          clear_line_buffer(strlen(buffer), pos);
          memset(buffer, '\0', bufsize * sizeof(char));
          // Copy command into buffer
          char *hcm = hist->curr->next->command;
          strcpy(buffer, hcm);
          // Set frontend accordingly
          printf("%s", hcm);
          pos = strlen(hcm);
          // Increment
          hist->curr = hist->curr->next;
        }
        break;
      case 66: // Down
        if (hist_iter == 0 || !hist->first) {
          break;
        } else if (hist->curr->prev) {
          // Clear buffer && line
          clear_line_buffer(strlen(buffer), pos);
          memset(buffer, '\0', bufsize * sizeof(char));
          // Copy command into buffer
          char *hcm = hist->curr->prev->command;
          strcpy(buffer, hcm);
          // Set frontend accordingly
          printf("%s", hcm);
          pos = strlen(hcm);
          // Decrement
          hist->curr = hist->curr->prev;
        } else if (!hist->curr->prev && strlen(buffer) > 0) {
          // Clear buffer && line
          clear_line_buffer(strlen(buffer), pos);
          memset(buffer, '\0', bufsize * (sizeof(char *)));
          pos = 0;
          hist_iter = 0;
        }
        break;
      case 67: // Right
        if (pos < strlen(buffer)) {
          putchar(buffer[pos++]);
        }
        break;
      case 68: // Left
        if (pos > 0) {
          putchar('\b');
          pos--;
        }
        break;
      }
      continue;
    } else if (c == '\t' || c == 9) {
      // TAB for completion... if not particular, inputs \t
      if (buffer[pos - 1] == ' ' || strlen(buffer) == 0 ||
          buffer[pos - 1] == '\t') {
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
        // clear_line_buffer(sizeof(buffer), pos);
        int b_len = strlen(f_tok);
        int final_char = strlen(buffer) - 1;
        for (int i = 0; i < b_len; i++) {
          buffer[final_char - i] = '\0';
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
 *
 * Also controls e.g. "wsh ls"
 */
int wsh_main(int argc, char **argv) {

  // environment init for home path
  // and username (for current user)
  HOME_PATH = getenv("HOME");
  char *uid = getenv("USER");

  if (!uid) {
    uid = "user";
  }

  if (!HOME_PATH) {
    HOME_PATH = "/";
  }

  // set up jobs list

  jobs_list = init_job_list();

  // init alias HT

  AL_TABLE = at_new_at(50);
  if (AL_TABLE == NULL) {
    fprintf(stderr, "{wsh @ init} -- ");
  }

  // Init builtins HT

  BI_TABLE = bt_new_bt(50);
  if (BI_TABLE == NULL) {
    fprintf(stderr, "{wsh @ init} -- ");
  }

  bt_set(BI_TABLE, "clear", &bin_clear);
  bt_set(BI_TABLE, "cd", &bin_cd);
  bt_set(BI_TABLE, "ln", &bin_ln);
  bt_set(BI_TABLE, "jobs", &bin_jobs);
  bt_set(BI_TABLE, "fg", &bin_fg);
  bt_set(BI_TABLE, "bg", &bin_bg);
  bt_set(BI_TABLE, "exit", &bin_exit);

  /**
   * Any hardcoded alias's should be placed
   * in the wshrc file; the function essentially
   * looks to add aliases and set environment things
   * e.g. home directory.
   *
   * Processing is based on spaces:
   *
   * home = /abc/xyz
   * alias l = ls -la
   *
   */

  // TODO -- .wshrc configuration
  // wsh_rc_init();

  /**
   * Headless execution... much faster since
   * there are far fewer initializations
   * needed to complete this.
   */
  if (argc > 1) {

    // Place the rest of argv inside of
    // tokens as needed.
    char **tokens = &argv[1];
    // the following is expensive during headless,
    // since additional alias's cannot be set up
    // (surely, setting up a wshrc file cannot be
    // too difficult)
    char **f_tokens = resolve_alias_shortcuts(tokens);
    char **argv = prep(f_tokens);

    printf("\n");
    if (!tokens) {
      // if error while parsing
      fprintf(stderr, "\n\t{wsh @ headless -- parsing} -- ran into generic "
                      "error parsing. exiting");
      exit(-1);
    }
    if (!f_tokens) {
      // if error while parsing
      fprintf(stderr, "\n\t{wsh @ headless -- resolving aliases and shortcuts} "
                      "-- ran into generic error parsing. exiting");
      exit(-1);
    }
    if (!argv) {
      fprintf(stderr, "\n\t{wsh @ headless -- prepping} -- ran into generic "
                      "error prepping. skipping");
      exit(-1);
    }
    // since defined, gives the length of each to be passed into everything.
    int tokct = ppstrlen(tokens);
    int f_tokct = ppstrlen(f_tokens);
    int argc = ppstrlen(argv);

    if (f_tokct > 0) {
      bin_builtin bi;
      if (strcmp(argv[0], "alias") == 0) {
        fprintf(stderr, "\n\t{wsh @ headless} -- cannot alias in headless");
      } else {
        execute(f_tokens, argv, 0);
      }
    }

    // Waiting for all background processes here:
    int wret, wstatus;
    while ((wret = waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED)) >
           0) {
      // examine all children who’ve terminated or stopped
      int wjid = get_job_jid(jobs_list, wret);
      if (WIFEXITED(wstatus)) {
        // terminated normally
        remove_job_pid(jobs_list, wret);
        if (printf("\n[%d] (%d) terminated with exit status %d\n", wjid, wret,
                   WEXITSTATUS(wstatus)) < 0) {
          fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
          exit(-1);
        }
      }
      if (WIFSIGNALED(wstatus)) {
        // terminated by signal
        remove_job_pid(jobs_list, wret);
        if (printf("\n[%d] (%d) terminated by signal %d\n", wjid, wret,
                   WTERMSIG(wstatus)) < 0) {
          fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
          exit(-1);
        }
      }
      if (WIFSTOPPED(wstatus)) {
        // stopped
        update_job_pid(jobs_list, wret, STOPPED);
        if (printf("\n[%d] (%d) suspended by signal %d\n", wjid, wret,
                   WSTOPSIG(wstatus)) < 0) {

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

    cleanup_job_list(jobs_list);
    return 0;
  }

  // splash screen
  if (splash() != 0) {
    fprintf(stderr, "{wsh @ init} -- failed splash screen");
  }

  // set up completions tree

  TrieNode *completions = tn_getNode();

  // add builtins
  tn_insert(completions, "cd");
  tn_insert(completions, "ln");
  tn_insert(completions, "bg");
  tn_insert(completions, "rm");
  tn_insert(completions, "jobs");
  tn_insert(completions, "fg");
  tn_insert(completions, "clear");
  tn_insert(completions, "exit");

  // Grab $PATH from env
  PATH_VAR = getenv("PATH");
  char pathvar_cpy[strlen(PATH_VAR)];
  pathvar_cpy[0] = 0;
  strcpy(pathvar_cpy, PATH_VAR);

  if (pathvar_cpy[0] != 0) {
    char *path;
    int i;

    // tokenize on colon to get paths
    // then use that immediately to
    // scandir, and add everything in
    // there to the completions system
    path = strtok(pathvar_cpy, ":");
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
        if (strcmp(curr, ".") == 0 || strcmp(curr, "..") == 0) {
          continue;
        } else if (notalpha(curr) || !str_islower(curr)) {
          continue;
        } else {
          tn_insert(completions, curr);
        }
      }
      for (i = 0; i < num_files; i++) {
        free(fListTemp[i]);
      }
      free(fListTemp);
      path = strtok(NULL, ":");
    }
  } else {
    fprintf(stderr, "{wsh @ init} -- $PATH variable could not be found?");
  }

  // Init the history data structures

  history *h = malloc(sizeof(history));
  h->first = NULL;
  h->curr = NULL;
  h->count = 0;

  char bang[80];
  strcpy(bang, "\n");
  strcat(bang, uid);
  strcat(bang, " @ wsh % ");

  // Start in user's home directory

  if (chdir(HOME_PATH) < 0) {
    perror("{wsh @ init chdir} ");
  }

  // REPL begins below

  for (;;) { // beginning of this = new command

    char cwd_path[PATH_MAX];
    cwd_path[0] = 0;

    if (getcwd(cwd_path, sizeof(cwd_path)) == NULL) {
      fprintf(stderr, "{wsh @ alias's and shortcuts} -- couldn't retrieve the "
                      "current working directory");
    }

    struct dirent **fListTemp;
    int num_files = scandir(cwd_path, &fListTemp, NULL, alphasort);

    int j;
    for (j = 0; j < num_files; j++) {
      char *curr = fListTemp[j]->d_name;
      int is_hidden = (strlen(curr) > 0) ? curr[0] == '.' : 1;
      if (strcmp(curr, ".") == 0 || strcmp(curr, "..") == 0 || is_hidden) {
        continue;
      } else if (notalpha(curr) || !str_islower(curr)) {
        continue;
      } else {
        tn_insert(completions, curr);
      }
    }

    if (printf("%s", bang) <
        0) { // scuffed try catch for when we reach the bottom of the screen.
      fprintf(stderr, "\n\t{wsh @ REPL} -- unable to write to screen");
      return -1;
    }

    if (fflush(stdout) < 0) {
      fprintf(stderr, "\n\t{wsh @ REPL} -- unable to flush stdout");
      return -1;
    }

    h->curr = h->first;
    char *buffer = get_line(h, completions);

    // tokenize
    char **tokens = wsh_tokenize(buffer);
    char **f_tokens = resolve_alias_shortcuts(tokens);
    char **argv = prep(f_tokens);
    printf("\n");
    if (!tokens) {
      // if error while parsing
      fprintf(stderr, "\n\t{wsh @ REPL -- parsing} -- ran into generic error "
                      "parsing. skipping");
      continue;
    }
    if (!f_tokens) {
      // if error while parsing
      fprintf(stderr, "\n\t{wsh @ REPL -- resolving aliases and shortcuts} -- "
                      "ran into generic error parsing. skipping");
      continue;
    }
    if (!argv) {
      fprintf(stderr, "\n\t{wsh @ REPL -- prepping} -- ran into generic error "
                      "prepping. skipping");
      continue;
    }
    // since defined, gives the length of each to be passed into everything.
    int tokct = ppstrlen(tokens);
    int f_tokct = ppstrlen(f_tokens);
    int argc = ppstrlen(argv);

    if (f_tokct > 0) {
      bin_builtin bi;
      if ((bi = bt_get(BI_TABLE, f_tokens[0])) != NULL) {
        (*bi)(f_tokct, f_tokens);
      } else if (strcmp(argv[0], "splash") == 0) {
        if (f_tokct > 1) {
          fprintf(stderr,
                  "\n\t{wsh @ splash} -- builtin does not take any arguments");
        }

        if (splash() != 0) {
          fprintf(stderr, "\n\t{wsh @ execute} -- splash failed to print");
          continue;
        }
      } else if (strcmp(argv[0], "alias") == 0) {
        process_alias(tokct, tokens, 0);
      } else {
        if (strcmp(ppstr_final(f_tokens), "&") == 0) {
          execute(f_tokens, argv, 1);
        } else {
          execute(f_tokens, argv, 0);
        }
      }
    }

    // Waiting for all background processes here:
    int wret, wstatus;
    while ((wret = waitpid(-1, &wstatus, WNOHANG | WUNTRACED | WCONTINUED)) >
           0) {
      // examine all children who’ve terminated or stopped
      int wjid = get_job_jid(jobs_list, wret);
      if (WIFEXITED(wstatus)) {
        // terminated normally
        remove_job_pid(jobs_list, wret);
        if (printf("[%d] (%d) terminated with exit status %d\n", wjid, wret,
                   WEXITSTATUS(wstatus)) < 0) {
          fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
          exit(-1);
        }
      }
      if (WIFSIGNALED(wstatus)) {
        // terminated by signal
        remove_job_pid(jobs_list, wret);
        if (printf("[%d] (%d) terminated by signal %d\n", wjid, wret,
                   WTERMSIG(wstatus)) < 0) {
          fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
          exit(-1);
        }
      }
      if (WIFSTOPPED(wstatus)) {
        // stopped
        update_job_pid(jobs_list, wret, STOPPED);
        if (printf("[%d] (%d) suspended by signal %d\n", wjid, wret,
                   WSTOPSIG(wstatus)) < 0) {

          fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
          exit(-1);
        }
      }
      if (WIFCONTINUED(wstatus)) {

        update_job_pid(jobs_list, wret, RUNNING);
        if (printf("[%d] (%d) resumed\n", wjid, wret) < 0) {
          fprintf(stderr, "{wsh @ REPL -- bg's} -- could not write out\n");
          exit(-1);
        }
      }
    }

    int k;
    for (k = 0; k < num_files; k++) {
      char *curr = fListTemp[k]->d_name;
      int is_hidden = (strlen(curr) > 0) ? curr[0] == '.' : 1;
      if (strcmp(curr, ".") == 0 || strcmp(curr, "..") == 0 || is_hidden) {
        continue;
      } else if (notalpha(curr) || !str_islower(curr)) {
        continue;
      } else {
        tn_remove(completions, curr, 0);
      }
      free(fListTemp[k]);
    }
    free(fListTemp);
  }

  cleanup_job_list(jobs_list);
  return 0;
}
