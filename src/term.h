#ifndef TERM_H__
#define TERM_H__

#include <errno.h>
#include <fcntl.h>
#include <termios.h>

int set_non_canonical_mode(int fd);
int reset_terminal_mode(int fd);
int set_non_blocking(int fd);

#define ESC "\033"
#define CLEAR_SCREEN ESC "[H" ESC "[J"

#ifdef TERM_IMPLEMENTATION
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
#endif // TERM_IMPLEMENTATION

#endif // TERM_H__
