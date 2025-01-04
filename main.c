#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define ESC "\033"
#define CLEAR_SCREEN ESC "[H" ESC "[J"
#define array_len(x) sizeof(x) / sizeof(x[0])

int set_non_canonical_mode(int fd);
int reset_terminal_mode(int fd);
int set_non_blocking(int fd);

#define ROWS 40
#define COLS 80
#define DEAD 0
#define ALIVE 1

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
  int board[ROWS * COLS];
  int next_board[ROWS * COLS];
} GameState;

int mod(int a, int b) { return (a % b + b) % b; }

int get_cell_index(GameState *game, int i, int j) { return j + game->cols * i; }

int get_cell(GameState *game, int i, int j) {
  i = mod(i, game->rows);
  j = mod(j, game->cols);
  return game->board[get_cell_index(game, i, j)];
}

void draw(FILE *stream, GameState *game) {
  printf(CLEAR_SCREEN);

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      if (i == game->cursor.y && j == game->cursor.x) {
        printf("X");
        continue;
      }

      int curr = get_cell(game, i, j);
      printf("%c", curr == ALIVE ? '#' : '.');
    }
    printf("\n");
  }

  fflush(stdout);
}

void toggle_cell(GameState *game) {
  int i = game->cursor.y;
  int j = game->cursor.x;

  int curr = get_cell(game, i, j);
  int cell_index = get_cell_index(game, i, j);
  game->board[cell_index] = curr == ALIVE ? DEAD : ALIVE;
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
      int neighbour = get_cell(game, row + di, col + dj);
      if (neighbour == ALIVE) {
        cnt++;
      }
    }
  }
  return cnt;
}
void evolve(GameState *game) {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      int cnt = count_neighbours(game, i, j);

      int curr = get_cell(game, i, j);
      int cell_index = get_cell_index(game, i, j);
      game->next_board[cell_index] = curr;

      if (curr == ALIVE && (cnt < 2 || cnt > 3))
        game->next_board[cell_index] = DEAD;

      if (curr == DEAD && cnt == 3)
        game->next_board[cell_index] = ALIVE;
    }
  }

  memcpy(game->board, game->next_board, sizeof(game->next_board));
}

GameState initGame() {
  GameState game = {
      .rows = ROWS,
      .cols = COLS,
      .cursor = {0},
      .running = false,
      .should_quit = false,
  };

  srand(time(NULL));

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      int cell_index = get_cell_index(&game, i, j);
      int random = (rand() % 3) + 1;
      game.board[cell_index] = random == 1 ? ALIVE : DEAD;
    }
  }

  memcpy(game.next_board, game.board, sizeof(game.board));

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

  uint timeout = 100000;

  char c;
  while (!game.should_quit) {
    int bytes_read = read(STDIN_FILENO, &c, 1);
    if (bytes_read == -1 && errno != EAGAIN) {
      perror("error stdin read");
      return EXIT_FAILURE;
    }

    draw(stdout, &game);
    usleep(timeout);

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
