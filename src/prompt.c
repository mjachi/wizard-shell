#include "prompt.h"

#define _BSD_SOURCE
//#define _POSIX_SOURCE
#define _XOPEN_SORUCE 700
#define UNUSED(x) (void)(x)
#define MAX_HOST 253
#define MAX_PATH 4096
#define BANG_MAX 64
#include "jobs.h"

extern int errno;
job_list_t *jobs_list;
int jcount;

/**
 * prompt.c 
 *
 * Contains I/O and REPL implementations.
 */



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
int splash(WINDOW *w) {
  int ret = 0;
  FILE *mural_ptr = fopen("../mural.txt", "r");
  if (!mural_ptr) {
    printw("Skipping splash; failed to find mural.txt");
    ret = -1;
  }

  //clear();
  //int c;
  //while ((c = getc(mural_ptr)) != EOF) {
  //  waddch(w,c);
  //  refresh();
  //}
  fclose(mural_ptr);
  printw("wsh (:wizard: shell) \n\n\n");

  refresh();
  
  return ret;
}


/**
 * Main REPL driving function.
 */
int awsh_main(int argc, char **argv, WINDOW* w) {
  UNUSED(argc);
  UNUSED(argv);

  /**
   * Find hostname, username, and if current user is a root or not.
   */
  char* t;
  char* uid = (t=getenv("USER")) ? t : "user";
  char host[MAX_HOST+1];
  if (gethostname(host, MAX_HOST+1) < 0) {
    strcpy(host, "wsh");
  }
  if (strlen(host) > 24) {
    strcpy(host, "wsh");
  }
  //char* path = (t=getenv("PATH")) ? t : "/bin/";
  //char* home = (t=getenv("HOME")) ? t : "/";
  //char* cwd = home; // alternatively set to getcwd to start from 
                    // location of the executable
  
  /** CLI stuff... add conditional for a flag?
   * send argv directly over...
   */

  char *bang = uid;
  strcat(bang, " @ wsh % ");

#ifdef PROMPT 
  
  //sigset_t old;
  //sigset_t full;
  //sigfillset(&full);
  
  // ignore signals while installing our handlers
  //sigprocmask(SIG_SETMASK, &full, &old);
  // Install signal handlers
  //if (install_handler(SIGINT, &sigint_handler))
    //perror("Warning: could not install handler for SIGINT");

  //if (install_handler(SIGTSTP, &sigtstp_handler))
    //perror("Warning: could not install handler for SIGTSTP");

  //if (install_handler(SIGQUIT, &sigquit_handler))
    //perror("Warning: could not install handler for SIGQUIT");
  // Restore the signal masks to previous values
  //sigprocmask(SIG_SETMASK, &old, NULL);


  signal(SIGINT, SIG_IGN);
  //signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  
  /**
   * Opening script... prints out whatever is in mural.txt.
   * Assumes width of 80 characters... obviously, one could 
   * change whatever's in there to have it print that out 
   * instead
   */
  //splash(w);
  
  // Init the history data structures
  
  history* h = malloc(sizeof(history));
  h->first = NULL;
  h->curr = NULL;
  h->count = 0;

  // Init the completions database
  




  // Beginning of REPL
  for (;;) {

    /**
     * - User can enter nothing and hit the return key (your terminal should 
     *   simply ignore the input and start a new line in the terminal, as Linux systems do).
     * - User should enter a supported command if there is any non-whitespace text.
     * - User can enter a redirection symbol at ANY point in a command (see 4
     *   for details on redirection), as long as there is a valid file following it
     * - User cannot use any more than one of the same redirect in one line (i.e. one input, one output).
     * - User can enter any amount of whitespace between tokens
     */
    if(wprintw(w, "\n%c @ wsh % ", uid) == ERR) {
      refresh();
      perror("\n{prompt.c//sh_main} Issue printing for REPL:");
      return -1;
    }
    refresh();

    // store input, then parse for it to be executed.
    char *buf = malloc(sizeof(char) * BUFF_SIZE);
    // recall from lab that this can just be done with a static
    // size array of char of a sufficiently large size
    if (!buf) {
      refresh();
      perror("\n{prompt.c//sh_main} Issue allocating for buffer");
      return -1;
    }

    char in;
    int pos = 0;
    
    // Next bit handles the user input
    //
    // conditionals to handle the various things I want to take
    // into consideration.
    while ((buf[pos] = in = getch()) != EOF) {
      if (in == '\n') {
        break;
      }
      
      if (in == KEY_BACKSPACE || in == KEY_DC) {
        // Captures both backspace and delete, so its keyboard agnostic
        pos--;
        delch();
        buf[pos] = 0;
      } else if (in == KEY_UP) {
        // Captures up arrow key
        wprintw(w, "\n You triggered going back in hist event \n");
      } else if (in == KEY_DOWN) {
        // Captures down arrow key
        wprintw(w, "\n You triggered going forward in hist event \n");
      } else if (in == KEY_LEFT) {
        // Captures left arrow key
        wprintw(w, "\n You triggered going left in buffer event \n");
      } else if (in == KEY_RIGHT) { 
        // Captures right arrow key
        wprintw(w, "\n You triggered going right in buffer event \n");
      } else if (in == KEY_STAB || in == '\t') {
        // Captures TAB press.
        wprintw(w, "\n You triggered TAB event \n");
      } else {
        // Default case
        addch(in);
        
      }

      refresh();
    }
  }
#endif


  cleanup_job_list(jobs_list);
  return 0; 
}

