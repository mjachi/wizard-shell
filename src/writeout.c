
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


