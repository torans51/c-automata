#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TERM_IMPLEMENTATION
#include "src/term.h"
#define GAME_IMPLEMENTATION
#include "src/game.h"

#define array_len(x) sizeof(x) / sizeof(x[0])

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

  uint timeout = 50000;
  // timeout = 200000;
  // timeout = 500000;

  char c;
  while (!game.should_quit) {
    int bytes_read = read(STDIN_FILENO, &c, 1);
    if (bytes_read == -1 && errno != EAGAIN) {
      perror("error stdin read");
      return EXIT_FAILURE;
    }

    draw(stdout, &game);

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
    case ' ':
      if (game.running)
        stop_game(&game);
      else
        start_game(&game);
      break;
    case 'R':
      randomize(&game);
      break;
    case 't':
      toggle_cell(&game);
      break;
    default:
      break;
    }

    c = '\0';

    usleep(timeout);
  }

  if (reset_terminal_mode(STDIN_FILENO) != EXIT_SUCCESS) {
    perror("reset_terminal_mode");
    return EXIT_FAILURE;
  }
}
