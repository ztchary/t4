#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MIN_COLS 50
#define MIN_ROWS 35

struct termios original_termios;

const char *too_small = "terminal is too small (%d x %d)";

const char *draws[] = {
	"\e[2C\e[1B          \e[1B\e[10D          \e[1B\e[10D          \e[1B\e[10D          \e[1B\e[10D          ",
	"\e[2C\e[1B▀▄      ▄▀\e[1B\e[10D  ▀▄  ▄▀  \e[1B\e[10D    ██    \e[1B\e[10D  ▄▀  ▀▄  \e[1B\e[10D▄▀      ▀▄",
	"\e[2C\e[1B ▄▄▀▀▀▀▄▄ \e[1B\e[10D▄▀      ▀▄\e[1B\e[10D█        █\e[1B\e[10D▀▄      ▄▀\e[1B\e[10D ▀▀▄▄▄▄▀▀ ",
};

const char *colors[] = {
	"\e[32m",
	"\e[31m",
	"\e[36m",
};

const int patterns[] = {
	0, 1, 2,
	3, 4, 5,
	6, 7, 8,
	0, 3, 6,
	1, 4, 7,
	2, 5, 8,
	0, 4, 8,
	2, 4, 6,
};

int x = 1;
int y = 1;
int turn = 1;
int win = 0;
int board[9] = {0};

bool board_full() {
	for (int i = 0; i < 9; i++) {
		if (board[i] == 0) return false;
	}
	return true;
}

int calc_win() {
	for (int i = 0; i < 24; i+=3) {
		if (board[patterns[i]] == 0) continue;
		if (board[patterns[i]] != board[patterns[i+1]]) continue;
		if (board[patterns[i+1]] != board[patterns[i+2]]) continue;
		return board[patterns[i]];
	}
	return 0;
}

void enter() {
	tcgetattr(STDIN_FILENO, &original_termios);

	struct termios tios = original_termios;
	tios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	tios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	tios.c_oflag &= ~(OPOST);
	tios.c_cflag |= (CS8);

	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 1;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tios);
	printf("\e[?1049h\e[?25l\e[H");
	fflush(stdout);
}

void leave() {
	printf("\e[?1049l\e[?25h");
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
	fflush(stdout);
}

void get_wsize(int *row, int *col) {
	struct winsize ws;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
	*row = ws.ws_row;
	*col = ws.ws_col;
}

void center_text(const char *text, int rows, int cols) {
	printf("\e[%d;%ldH%s", rows/2 - 14, cols/2 - strlen(text)/2, text);
}

void draw() {
	int rows, cols;
	get_wsize(&rows, &cols);
	printf("\e[H\e[J");
	if (rows < MIN_ROWS || cols < MIN_COLS) {
		printf("\e[%d;%ldH", rows / 2, cols / 2 - (strlen(too_small) / 2));
		printf(too_small, cols, rows);
		fflush(stdout);
		return;
	}

	int xoff = cols/2 - 23;
	int yoff = rows/2 - 11;

	printf("\e[107m");
	printf("\e[%d;%dH%*s", yoff + 7, xoff, 46, " ");
	printf("\e[%d;%dH%*s", yoff + 15, xoff, 46, " ");
	printf("\e[%d;%dH", yoff, xoff + 14);
	for (int i = 0; i < 23; i++) printf("  \e[1B\e[2D");
	printf("\e[%d;%dH", yoff, xoff + 30);
	for (int i = 0; i < 23; i++) printf("  \e[1B\e[2D");

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			printf("\e[%dm", (i == y && j == x) * 47);
			printf("\e[%d;%dH", yoff + (i * 8), xoff + (j * 16));
			printf("%s%s", colors[board[j+i*3]], draws[board[j+i*3]]);
		}
	}

	printf("\e[m");

	if (win == 1) center_text("X wins!", rows, cols);
	if (win == 2) center_text("O wins!", rows, cols);
	if (win == 3) center_text("Draw!", rows, cols);

	for (int i = 0; i < 3; i++) {
		if (board[i] != 0 && board[i] == board[i+3] && board[i] == board[i+6]) {
			printf("\e[%d;%dH", yoff - 1, xoff + i * 16 + 5);
			printf("%s", colors[board[i]]);
			for (int j = 0; j < 25; j++) printf(" ██ \e[1B\e[4D");
		}
		if (board[i*3] != 0 && board[i*3] == board[i*3+1] && board[i*3] == board[i*3+2]) {
			printf("\e[%d;%dH", yoff + i * 8 + 2, xoff - 2);
			printf("%s", colors[board[i*3]]);
			printf("%*s\e[1B\e[50D", 50, "");
			for (int j = 0; j < 50; j++) printf("█");
			printf("\e[1B\e[50D%*s", 50, "");
		}
	}

	if (board[0] != 0 && board[0] == board[4] && board[0] == board[8]) {
		printf("\e[%d;%dH", yoff - 1, xoff - 2);
		printf("%s", colors[board[0]]);
		printf("██▄  ");
		for (int i = 0; i < 23; i++) printf("\e[1B\e[6D  ▀██▄  ");
		printf("\e[1B\e[4D▀██");
	}

	if (board[2] != 0 && board[2] == board[4] && board[2] == board[6]) {
		printf("\e[%d;%dH", yoff + 23, xoff - 2);
		printf("%s", colors[board[2]]);
		printf("██▀  ");
		for (int i = 0; i < 23; i++) printf("\e[1A\e[6D  ▄██▀  ");
		printf("\e[1A\e[4D▄██");
	}

	printf("\e[m");

	fflush(stdout);
}

void winch(int sg) {
	(void)sg;
	draw();
}

int main() {
	signal(SIGWINCH, winch);
	enter();

	char c, o;
	while (c != 'q') {
		draw();
		o = read(STDIN_FILENO, &c, 1);
		if (o != 1) continue;
		if (win != 0) {
			memset(board, 0, sizeof(board));
			win = 0;
			continue;
		}
		switch (c) {
			case 'h': x--; break;
			case 'j': y++; break;
			case 'k': y--; break;
			case 'l': x++; break;
			case 'r': memset(board, 0, sizeof(board)); break;
			case '\r': case ' ': {
				if (board[x+y*3] != 0) break;
				board[x+y*3] = turn;
				turn = 1 + (turn & 1);
				break;
			}
		}
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x > 2) x = 2;
		if (y > 2) y = 2;
		win = calc_win();
		if (win == 0 && board_full()) win = 3;
	}

	leave();
}

