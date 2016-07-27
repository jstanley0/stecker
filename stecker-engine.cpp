#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <iostream>

#define MAX_WIDTH 16
#define MAX_BOARD_SIZE (MAX_WIDTH * MAX_WIDTH + 1)

class GameState
{
  int rows, cols;
  char to_play;
  char board[MAX_BOARD_SIZE];

public:
  GameState() : rows(0), cols(0), to_play('+') {}
  explicit GameState(const GameState& rhs) :
    rows(rhs.rows), cols(rhs.cols), to_play(rhs.to_play)
  {
    memcpy(board, rhs.board, rows * cols);
  }

  inline int width() { return cols; }
  inline int height() { return rows; }

  bool read()
  {
    char buf[3];
    fgets(buf, 3, stdin);
    rows = atoi(buf);
    fgets(buf, 3, stdin);
    cols = atoi(buf);
    if (cols > MAX_WIDTH || rows * cols > MAX_BOARD_SIZE)
    {
      fprintf(stderr, "Board too large\n");
      exit(1);
    }
    if (rows * cols == 0)
      return false;
    fgets(board, rows * cols + 1, stdin);
    return true;
  }

  inline char &cell(int row, int col)
  {
    return board[row * cols + col];
  }

  bool column_full(int col)
  {
    return cell(0, col) != '0';
  }

  int check_state(int play_column)
  {
    int col0 = play_column - 3;
    if (col0 < 0) col0 = 0;
    int col1 = play_column + 3;
    if (col1 > cols) col1 = cols;

    for(int row = 0; row < rows; ++row)
    {
      for(int col = col0; col < col1; ++col)
      {
        char c = cell(row, col);
        if (c != '0')
        {
          // horizontal
          if (col <= cols - 4 && cell(row, col + 1) == c && cell(row, col + 2) == c && cell(row, col + 3) == c)
            return c;
          // vertical
          if (row <= rows - 4 && cell(row + 1, col) == c && cell(row + 2, col) == c && cell(row + 3, col) == c)
            return c;
          // slash
          if (col >= 3 && row <= rows - 4 && cell(row + 1, col - 1) == c && cell(row + 2, col - 2) == c && cell(row + 3, col - 3) == c)
            return c;
          // backslash
          if (col <= cols - 4 && row <= rows - 4 && cell(row + 1, col + 1) == c && cell(row + 2, col + 2) == c && cell(row + 3, col + 3) == c)
            return c;
        }
      }
    }
    return '0';
  }

  int make_play(int col)
  {
    int row = rows - 1;
    while (row >= 0)
    {
      if (cell(row, col) == '0')
      {
        cell(row, col) = to_play;
        to_play = (to_play == '+') ? '-' : '+';
        return col;
      }
      --row;
    }
    fprintf(stderr, "Invalid move attempted. :(\n");
    exit(1);
  }

  int find_winning_play()
  {
    for(int j = 0; j < cols; ++j)
    {
      if (!column_full(j))
      {
        GameState countersim(*this);
        char turn = to_play;
        countersim.make_play(j);
        if (countersim.check_state(j) == turn)
          return j;
      }
    }
    return -1;
  }

  int make_randomish_play()
  {
    // if there is a winning move here, make it.
    int col = find_winning_play();
    if (col >= 0)
      return make_play(col);

    // otherwise play randomly
    int legal_moves[MAX_WIDTH];
    int count = 0;
    for(int i = 0; i < cols; ++i)
      if (!column_full(i))
        legal_moves[count++] = i;
    if (count == 0)
      return 0;
    return make_play(legal_moves[rand() % count]);
  }
};

struct StateInfo
{
  GameState *game;
  int col;
  int iterations;
  int *wins;
  pthread_t thread;
};

void *column_thread(void *param)
{
  StateInfo *info = (StateInfo *)param;
  for(int i = 0; i < info->iterations; ++i)
  {
    GameState sim(*(info->game));
    sim.make_play(info->col);

    // on the first iteration, short-circuit a win
    // and avoid the play if it opens an immediate loss
    if (i == 0)
    {
      if (sim.check_state(info->col) == '+')
      {
        *(info->wins) = info->iterations;
        break;
      }
      else if(sim.find_winning_play() >= 0)
      {
        break;
      }
    }

    for(;;)
    {
      char result = sim.check_state(info->col);
      if (result == '+')
        ++(*(info->wins));
      if (result != '0')
        break;
      if (!sim.make_randomish_play())
        break;
    }
  }
  return NULL;
}

void run_simulations(GameState &game, int wins[])
{
  int iterations = 350000 / (game.width() * game.height());
  fprintf(stderr, "Using %d iterations\n", iterations);

  StateInfo thread_states[MAX_WIDTH];
  for(int col = 0; col < game.width(); ++col)
  {
    if (game.column_full(col))
      continue; // column full

    thread_states[col].game = &game;
    thread_states[col].col = col;
    thread_states[col].iterations = iterations;
    thread_states[col].wins = &wins[col];
    pthread_create(&thread_states[col].thread, NULL, column_thread, &thread_states[col]);
  }
  for(int col = 0; col < game.width(); ++col)
  {
    pthread_join(thread_states[col].thread, NULL);
  }
}

void play(GameState &game)
{
  int wins[MAX_WIDTH];
  for(int i = 0; i < MAX_WIDTH; ++i)
    wins[i] = 0;
  run_simulations(game, wins);
  int max_wins = -1, win_idx = -1;
  for(int i = 0; i < game.width(); ++i)
  {
    fprintf(stderr, "%d ", wins[i]);
    if (wins[i] > max_wins)
    {
      max_wins = wins[i];
      win_idx = i;
    }
  }
  fprintf(stderr, "\n");
  printf("%02d", win_idx);
  fflush(stdout);
}

int main(int argc, char **argv)
{
  srand(time(0));
  GameState game;
  while(game.read())
  {
    play(game);
  }
  return 0;
}

