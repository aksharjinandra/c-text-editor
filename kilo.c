#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

struct termios orig_termios; 

void die(const char *s) {
    perror(s); 
    exit(1); 
}

void disableRawMode(void) {
    if (tcsetattr(STDIN_FILENO , TCSAFLUSH ,&orig_termios) == -1)
        die ("tcsetattr");
}

void enableRawMode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr"); 
    atexit(disableRawMode);

    struct termios raw = orig_termios; 
    raw.c_iflag &= ~(BRKINT| ICRNL | INPCK | ISTRIP | IXON); 
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); 
    raw.c_cc[VMIN] = 0; 
    raw.c_cc[VTIME] = 10; 

    tcsetattr(STDERR_FILENO, TCSAFLUSH, &raw);
}

int main(void) { 
    enableRawMode(); 
    while (1) {
        char c = '\0'; 
        read(STDIN_FILENO, &c, 1); 
        if (iscntrl(c)) {
            printf("%d\r\n", c); 
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
        if (c == 'q') break; 
    }                                                            
    return 0; 
    
}
