#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>

#define WIDTH 40
#define HEIGHT 20
#define PADDLE_H 4
#define FPS 30

struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int kbhit() {
    char buf;
    int n = read(STDIN_FILENO, &buf, 1);
    if (n == 1) return buf;
    return 0;
}

void draw(int ball_x, int ball_y, int paddle1_y, int paddle2_y, int score1, int score2) {
    printf("\x1b[H"); // cursor home
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (x == 0 && y >= paddle1_y && y < paddle1_y + PADDLE_H)
                putchar('|');
            else if (x == WIDTH-1 && y >= paddle2_y && y < paddle2_y + PADDLE_H)
                putchar('|');
            else if (x == ball_x && y == ball_y)
                putchar('O');
            else
                putchar(' ');
        }
        putchar('\n');
    }
    printf("Score: %d - %d\n", score1, score2);
    fflush(stdout);
}

int main() {
    enable_raw_mode();
    printf("\x1b[2J"); // clear screen

    int paddle1_y = HEIGHT/2 - PADDLE_H/2;
    int paddle2_y = HEIGHT/2 - PADDLE_H/2;
    int ball_x = WIDTH/2, ball_y = HEIGHT/2;
    int ball_dx = 1, ball_dy = 1;
    int score1 = 0, score2 = 0;

    struct timespec ts = {0, (long)(1e9/FPS)};

    while (1) {
        int key = kbhit();
        if (key == 'q') break;
        if (key == 'w' && paddle1_y>0) paddle1_y--;
        if (key == 's' && paddle1_y<PADDLE_H+HEIGHT-PADDLE_H-1) paddle1_y++;
        if (key == 'A' && paddle2_y>0) paddle2_y--; // arrow up
        if (key == 'B' && paddle2_y<PADDLE_H+HEIGHT-PADDLE_H-1) paddle2_y++; // arrow down

        ball_x += ball_dx;
        ball_y += ball_dy;

        // collision with top/bottom
        if (ball_y <= 0 || ball_y >= HEIGHT-1) ball_dy *= -1;

        // collision with paddles
        if (ball_x == 1 && ball_y >= paddle1_y && ball_y < paddle1_y + PADDLE_H) ball_dx *= -1;
        if (ball_x == WIDTH-2 && ball_y >= paddle2_y && ball_y < paddle2_y + PADDLE_H) ball_dx *= -1;

        // scoring
        if (ball_x < 0) { score2++; ball_x = WIDTH/2; ball_y = HEIGHT/2; ball_dx = 1; }
        if (ball_x >= WIDTH) { score1++; ball_x = WIDTH/2; ball_y = HEIGHT/2; ball_dx = -1; }

        draw(ball_x, ball_y, paddle1_y, paddle2_y, score1, score2);
        nanosleep(&ts, NULL);
    }

    disable_raw_mode();
    printf("\x1b[?25h"); // show cursor
    return 0;
}
