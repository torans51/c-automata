#ifndef GAME_H__
#define GAME_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "term.h"

#define ROWS 60
#define COLS 300
#define DEAD 0
#define ALIVE 1
#define REFRESH_RATE 60 // refresh rate for draw (fps)
#define UPDATE_RATE 10  // evolve frame rate (fps)

typedef struct {
  int x;
  int y;
} Vec2;

enum Direction { UP, DOWN, LEFT, RIGHT };

typedef struct {
  Vec2 cursor;
  int rows;
  int cols;
  bool running;
  bool should_quit;
  int board[ROWS * COLS];
  int next_board[ROWS * COLS];
  long prev_t;      // millisec
  long draw_prev_t; // millisec
} GameState;

int mod(int a, int b);
GameState initGame();
int get_cell_index(GameState *game, int i, int j);
int get_cell(GameState *game, int i, int j);
void toggle_cell(GameState *game);
void start_game(GameState *game);
void stop_game(GameState *game);
void evolve(time_t t, GameState *game);
void randomize(GameState *game);
void draw(time_t t, FILE *stream, GameState *game);

#ifdef GAME_IMPLEMENTATION
int mod(int a, int b) { return (a % b + b) % b; }

int get_cell_index(GameState *game, int i, int j) { return j + game->cols * i; }

int get_cell(GameState *game, int i, int j) {
  i = mod(i, game->rows);
  j = mod(j, game->cols);
  return game->board[get_cell_index(game, i, j)];
}

void move(GameState *game, enum Direction dir) {
  int dx = 0;
  int dy = 0;
  switch (dir) {
  case UP:
    dy--;
    break;
  case DOWN:
    dy++;
    break;
  case LEFT:
    dx--;
    break;
  case RIGHT:
    dx++;
    break;
  }
  game->cursor.x = mod(game->cursor.x + dx, game->cols);
  game->cursor.y = mod(game->cursor.y + dy, game->rows);
}

void draw(time_t t, FILE *stream, GameState *game) {
  if (game->draw_prev_t == 0) {
    game->draw_prev_t = t;
    return;
  }

  time_t dt = t - game->draw_prev_t;
  if (REFRESH_RATE * dt <= 1000)
    return;

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
  game->draw_prev_t = t;
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

void evolve(time_t t, GameState *game) {
  if (game->prev_t == 0) {
    game->prev_t = t;
    return;
  }

  time_t dt = t - game->prev_t;
  if (UPDATE_RATE * dt <= 1000)
    return;

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
  game->prev_t = t;
}

void randomize(GameState *game) {
  srand(time(NULL));

  for (int i = 0; i < game->rows; i++) {
    for (int j = 0; j < game->cols; j++) {
      int cell_index = get_cell_index(game, i, j);
      int random = (rand() % 10) + 1;
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
      .prev_t = 0,
      .draw_prev_t = 0,
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
#endif // GAME_IMPLEMENTATION

#endif // GAME_H__
