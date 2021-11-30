
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

void back_up_buffer(int);
void clear_line_buffer(int, int);
int getch(void);


// essentially a backspace
void back_up_buffer(int n){
  for (int i=0; i < n; i++) putchar('\b');
}

// Print n spaces
void whitespace(int n) {
  for (int i=0; i < n; i++) putchar(' ');
}

// Clear length len at position p
void clear_line_buffer(int len, int p) {
  back_up_buffer(p);
  whitespace(len);
  back_up_buffer(len);
}

// Same behavior but without ncurses, etc
int getch(void){
    int ch;
    struct termios oldt;
    struct termios newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); 
    ch = getchar(); 
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); 
    return ch; 
}


// Print out an array of strings
int print_arr(char **out) {
  if (!out) {
    return -1;
  }
  
  int i = 0;

  while (out[i]) {
    if (printf("%s ", out[i]) < 0) {
      fprintf(stderr, "{wsh @ print_arr} -- failed to print out the %d'th string in the array", i);
      return -1;
    }
    i++;
  }
  if (printf("\n") < 0) {
    fprintf(stderr, "\n\t{wsh @ print_arr} -- failed to print new line");
    return -1;
  }
  return 0;
}


