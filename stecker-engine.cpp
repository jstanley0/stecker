#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_BOARD_SIZE (16*16)

class GameState
{
  int rows, cols;
  char board[MAX_BOARD_SIZE];

public:
  bool read()
  {
    char buf[3];
    fgets(buf, 3, stdin);
    rows = atoi(buf);
    fgets(buf, 3, stdin);
    cols = atoi(buf);
    if (rows * cols > MAX_BOARD_SIZE)
    {
      fprintf(stderr, "Board too large\n");
      exit(1);
    }
    if (rows * cols == 0)
      return false;
    fgets(board, rows * cols + 1, stdin);
    return true;
  }

  void play()
  {
    // FIXME implement!
    //fprintf(stderr, "%s\n", board);
    printf("%02d", 0);
    fflush(stdout);
  }

};

int main(int argc, char **argv)
{
  GameState game;
  while(game.read())
  {
    game.play();
  }
  return 0;
}

