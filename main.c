#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
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

  // hard limit for timeout to avoid 100% cpu
  uint timeout = 10000;

  char c;
  struct timeval now;
  while (!game.should_quit) {
    gettimeofday(&now, NULL);
    long t = (long)(now.tv_sec) * 1000 + (now.tv_usec / 1000);

    int bytes_read = read(STDIN_FILENO, &c, 1);
    if (bytes_read == -1 && errno != EAGAIN) {
      perror("error stdin read");
      return EXIT_FAILURE;
    }

    draw(t, stdout, &game);
    if (game.running) {
      evolve(t, &game);
    }

    switch (c) {
    case 'q':
      game.should_quit = true;
      break;
    case 'k':
      move(&game, UP);
      break;
    case 'j':
      move(&game, DOWN);
      break;
    case 'h':
      move(&game, LEFT);
      break;
    case 'l':
      move(&game, RIGHT);
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
