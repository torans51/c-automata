#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define ESC "\033"
#define CLEAR_SCREEN ESC "[H" ESC "[J"
#define array_len(x) sizeof(x) / sizeof(x[0])

int set_non_canonical_mode(int fd);
int reset_terminal_mode(int fd);
int set_non_blocking(int fd);

#define ROWS 10
#define COLS 10

typedef struct {
  int x;
  int y;
} Vec2;

typedef struct {
  Vec2 cursor;
  int rows;
  int cols;
  bool running;
  bool should_quit;
  char board[ROWS * COLS];
} GameState;

int mod(int a, int b) { return (a % b + b) % b; }

void draw(GameState *game) {
  printf(CLEAR_SCREEN);

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (i == game->cursor.y && j == game->cursor.x) {
        printf("X");
        continue;
      }

      printf("%c", game->board[j + game->cols * i]);
    }
    printf("\n");
  }
}

void toggle_cell(GameState *game) {
  int i = game->cursor.y;
  int j = game->cursor.x;

  game->board[j + game->cols * i] = '#';
}

void start_game(GameState *game) { game->running = true; }
void stop_game(GameState *game) { game->running = false; }

int count_neighbours(GameState *game, int row, int col) {
  int cnt = 0;
  for (int di = -1; di <= 1; di++) {
    for (int dj = -1; dj <= 1; dj++) {
      if (di == 0 && dj == 0)
        continue;

      int i = mod(row + di, game->rows);
      int j = mod(col + dj, game->cols);
      char neighbour = game->board[j + game->cols * i];
      if (neighbour == '#') {
        cnt++;
      }
    }
  }
  return cnt;
}
void evolve(GameState *game) {
  char new_board[game->rows * game->cols + 1];
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      int cnt = count_neighbours(game, i, j);

      char curr = game->board[j + game->cols * i];
      new_board[j + game->cols * i] = curr;

      if (curr == '#' && (cnt < 2 || cnt > 3))
        new_board[j + game->cols * i] = '.';

      if (curr == '.' && cnt == 3)
        new_board[j + game->cols * i] = '#';
    }
  }

  memcpy(game->board, new_board, sizeof(new_board));
}

GameState initGame() {
  GameState game = {
      .rows = ROWS,
      .cols = COLS,
      .cursor = {0},
      .running = false,
      .should_quit = false,
  };

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      game.board[j + game.cols * i] = '.';
    }
  }

  return game;
}

int main() {
  GameState game = initGame();

  if (set_non_canonical_mode(STDIN_FILENO) != EXIT_SUCCESS) {
    perror("set_non_canonical_mode");
    return EXIT_FAILURE;
  }

  if (set_non_blocking(STDIN_FILENO) != EXIT_SUCCESS) {
    perror("set_non_blocking");
    return EXIT_FAILURE;
  }

  uint timeout = 200000;

  char c;
  while (!game.should_quit) {
    int bytes_read = read(STDIN_FILENO, &c, 1);
    if (bytes_read == -1 && errno != EAGAIN) {
      perror("error stdin read");
      return EXIT_FAILURE;
    }

    // draw_cursor(buffer, &curr_pos);
    draw(&game);
    fflush(stdout);
    usleep(timeout);

    // if (bytes_read == -1) {
    //   if (errno == EAGAIN) {
    //     usleep(timeout);
    //     continue;
    //   }
    //
    // } else if (bytes_read == 0) {
    //   usleep(timeout);
    // }

    if (game.running) {
      evolve(&game);
    }

    switch (c) {
    case 'q':
      game.should_quit = true;
      break;
    case 'j':
      game.cursor.y = mod(game.cursor.y + 1, game.rows);
      break;
    case 'k':
      game.cursor.y = mod(game.cursor.y - 1, game.rows);
      break;
    case 'h':
      game.cursor.x = mod(game.cursor.x - 1, game.cols);
      break;
    case 'l':
      game.cursor.x = mod(game.cursor.x + 1, game.cols);
      break;
    case 'p':
      start_game(&game);
      break;
    case 's':
      stop_game(&game);
      break;
    case ' ':
      toggle_cell(&game);
      break;
    default:
      break;
    }

    c = '\0';
  }

  if (reset_terminal_mode(STDIN_FILENO) != EXIT_SUCCESS) {
    perror("reset_terminal_mode");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int set_non_canonical_mode(int fd) {
  struct termios tty;

  if (tcgetattr(fd, &tty) == -1) {
    perror("cannot read terminal attributes");
    return EXIT_FAILURE;
  }

  tty.c_lflag = tty.c_lflag & ~(ICANON | ECHO);
  tty.c_cc[VMIN] = 1;  // Minimum number of characters to read
  tty.c_cc[VTIME] = 0; // No timeout

  if (tcsetattr(fd, TCSANOW, &tty) == -1) {
    perror("cannot write terminal attributes");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int reset_terminal_mode(int fd) {
  struct termios tty;

  if (tcgetattr(fd, &tty) == -1) {
    perror("cannot read stdin attributes");
    return EXIT_FAILURE;
  }

  tty.c_lflag = tty.c_lflag | (ICANON | ECHO);

  if (tcsetattr(fd, TCSANOW, &tty) == -1) {
    perror("cannot set attributes for stdin");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int set_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("cannot read stdin flags");
    return EXIT_FAILURE;
  }

  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("cannot set nonblock flag for stdin");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
