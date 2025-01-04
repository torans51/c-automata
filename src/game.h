#ifndef GAME_H__
#define GAME_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "term.h"

#define ROWS 50
#define COLS 200
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

int mod(int a, int b);
GameState initGame();
int get_cell_index(GameState *game, int i, int j);
int get_cell(GameState *game, int i, int j);
void toggle_cell(GameState *game);
void start_game(GameState *game);
void stop_game(GameState *game);
void evolve(GameState *game);
void randomize(GameState *game);
void draw(FILE *stream, GameState *game);

#ifdef GAME_IMPLEMENTATION
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
        fputs("X", stream);
        continue;
      }

      int curr = get_cell(game, i, j);
      fputs(curr == ALIVE ? "#" : ".", stream);
    }
    fputs("\n", stream);
  }

  fflush(stream);
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

void randomize(GameState *game) {
  srand(time(NULL));

  for (int i = 0; i < game->rows; i++) {
    for (int j = 0; j < game->cols; j++) {
      int cell_index = get_cell_index(game, i, j);
      int random = (rand() % 15) + 1;
      game->board[cell_index] = random == 1 ? ALIVE : DEAD;
    }
  }
}

GameState initGame() {
  GameState game = {
      .rows = ROWS,
      .cols = COLS,
      .cursor = {0},
      .running = false,
      .should_quit = false,
  };

  for (int i = 0; i < game.rows; i++) {
    for (int j = 0; j < game.cols; j++) {
      int cell_index = get_cell_index(&game, i, j);
      game.board[cell_index] = DEAD;
    }
  }

  memcpy(game.next_board, game.board, sizeof(game.board));

  return game;
}
#endif

#endif
