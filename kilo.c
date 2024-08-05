#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h> 

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig {
    int screenrows; 
    int screencols; 

    struct termios orig_termios; 
};

struct editorConfig E; 

/*** terminal ***/
void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4); 
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s); 
    exit(1); 
}

void disableRawMode(void) {
    if (tcsetattr(STDIN_FILENO , TCSAFLUSH , &E.orig_termios) == -1)
        die ("tcsetattr");
}

void enableRawMode(void) {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr"); 
    atexit(disableRawMode);

    struct termios raw = E.orig_termios; 
    raw.c_iflag &= ~(BRKINT| ICRNL | INPCK | ISTRIP | IXON); 
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); 
    raw.c_cc[VMIN] = 0; 
    raw.c_cc[VTIME] = 10; 

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey(void) {
    int nread; 
    char c; 
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die ("read"); 
    }
    return c; 
}


int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}


int getWindowSize(int *rows, int *cols) {
    struct winsize ws; 

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999c\x1b[999B", 12) != 12) return -1; 
        return getCursorPosition(rows, cols); 
    } else {
        *cols = ws.ws_col; 
        *rows = ws.ws_row; 
        return 0; 
    }
}

/*** append buffer ***/

struct abuf { 
    char *b; 
    int len; 
}; 

#define ABUF_INIT {NULL, 0} 

/*adds new string to an existing buffer*/
void abAppend(struct abuf *buffer, const char *appendString, int appendLength) {
    char *newBuffer = realloc(buffer->b, buffer->len + appendLength); 

    if (newBuffer == NULL) return; 
    memcpy(newBuffer + buffer->len, appendString, appendLength);  
    buffer->b = newBuffer; 
    buffer->len += appendLength; 
}

/*** output ***/

void editorDrawRows(void) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        if (y == E.screenrows - 1) {
            // Handle the last row separately
            write(STDOUT_FILENO, "~", 1);
        } else {
            // For all other rows, print "~" and move to the next line
            write(STDOUT_FILENO, "~\r\n", 3);
        }
    }
}


void editorRefreshScreen(void) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows(); 
    write(STDOUT_FILENO, "\x1b[H", 3);

}

/*** input ***/
void editorProcessKeypress(void) {
    char c = editorReadKey(); 

    switch(c) {
        case CTRL_KEY('q'): 
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0); 
            break; 
    }
}

/*** init ***/

void initEditor(void) {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die ("getWindowSize"); 
}

int main(void) {    
    enableRawMode();
    initEditor(); 

    while (1) {
        editorRefreshScreen(); 
        editorProcessKeypress(); 
    }                                                            
    return 0; 
    
}
