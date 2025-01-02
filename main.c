#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define ESC "\033"
#define CLEAR_SCREEN ESC "[H" ESC "[J"

int set_non_canonical_mode(int fd);
int reset_terminal_mode(int fd);
int set_non_blocking(int fd);

int main() {
  printf(CLEAR_SCREEN);

  printf("type 'q' to exit ...");
  fflush(stdout);

  if (set_non_canonical_mode(STDIN_FILENO) != EXIT_SUCCESS) {
    perror("set_non_canonical_mode");
    return EXIT_FAILURE;
  }

  if (set_non_blocking(STDIN_FILENO) != EXIT_SUCCESS) {
    perror("set_non_blocking");
    return EXIT_FAILURE;
  }

  bool should_quit = false;
  uint timeout = 200000;

  char c;
  while (!should_quit) {
    int bytes_read = read(STDIN_FILENO, &c, 1);
    if (bytes_read == -1) {
      if (errno == EAGAIN) {
        usleep(timeout);
        continue;
      }

      perror("error stdin read");
      return EXIT_FAILURE;
    } else if (bytes_read == 0) {
      usleep(timeout);
    }

    if (c == 'q') {
      should_quit = true;
    }
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
