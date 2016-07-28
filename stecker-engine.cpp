#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <string>

#define MAX_WIDTH 14
#define MAX_BOARD_SIZE (MAX_WIDTH * MAX_WIDTH)

int g_Ply = 3;
int g_Trials = 100;
void tune_depth(int width)
{
  if (width <= 7)
    g_Ply = 4;
  else
    g_Ply = 3;

  if (width <= 10)
    g_Trials = 100;
  else if (width <= 12)
    g_Trials = 75;
  else
    g_Trials = 50;
}

char opponent(char player)
{
  return (player == '1') ? '2' : '1';
}

class GameState
{
  int rows, cols;
  int expected_result;
  char to_play;
  char board[MAX_BOARD_SIZE];

  int run_length(char c, int row, int col, int dr, int dc) const
  {
    int length = 0;
    while(cell(row, col) == c)
    {
      if (++length == 3)
        break;

      row += dr;
      if (row < 0 || row >= rows)
        break;

      col += dc;
      if (col < 0 || col >= cols)
        break;
    }
    return length;
  }

public:
  GameState() : rows(0), cols(0), to_play('0'), expected_result(-1) {}

  GameState(int _rows, int _cols, char _to_play) : rows(_rows), cols(_cols), to_play(_to_play), expected_result(-1)
  {
    memset(board, '0', rows * cols);
  }

  explicit GameState(const GameState& rhs) :
    rows(rhs.rows), cols(rhs.cols), expected_result(rhs.expected_result), to_play(rhs.to_play)
  {
    memcpy(board, rhs.board, rows * cols);
  }

  inline int width() const { return cols; }
  inline int height() const { return rows; }
  inline char turn() const { return to_play; }
  inline int test_value() const { return expected_result; }

  // protocol (newline separated)
  // [optional] !test_result (expected column to play in)
  // player # 1 or 2; 0 means exit (game over)
  // height
  // width
  // board (`height` lines of width `width` containing '0', '1', and '2')
  // the column to play in is returned
  bool read()
  {
    std::string line;
    std::getline(std::cin, line);
    if (line[0] == '!')
    {
      expected_result = atoi(line.c_str() + 1);
      std::getline(std::cin, line);
    }
    to_play = line[0];
    if (to_play == '0')
      return false;

    std::getline(std::cin, line);
    rows = atoi(line.c_str());

    std::getline(std::cin, line);
    cols = atoi(line.c_str());

    if (cols > MAX_WIDTH || rows * cols > MAX_BOARD_SIZE) {
      std::cerr << "Board too large\n";
      exit(1);
    }
    if (rows * cols == 0)
      return false;

    char *row_data = &board[0];
    for(int row = 0; row < rows; ++row)
    {
      std::getline(std::cin, line);
      if (line.size() != cols) {
        std::cerr << "Invalid line length: expected " << cols << "columns:\n" << line << std::endl;
        exit(1);
      }
      memcpy(row_data, line.c_str(), cols);
      row_data += cols;
    }

    return true;
  }

  inline char cell(int row, int col) const
  {
    return board[row * cols + col];
  }

  inline char &cell(int row, int col)
  {
    return board[row * cols + col];
  }

  inline bool column_full(int col) const
  {
    return cell(0, col) != '0';
  }

  // test whether the piece just played forms a win
  // return the player name ('1' or '2') or '0' otherwise
  char check_state(int play_row, int play_col) const
  {
    char c = cell(play_row, play_col);
    if (c != '0')
    {
      // horizontal
      if (1 + run_length(c, play_row, play_col - 1, 0, -1) + run_length(c, play_row, play_col + 1, 0, 1) >= 4)
        return c;

      // vertical
      if (1 + run_length(c, play_row - 1, play_col, -1, 0) + run_length(c, play_row + 1, play_col, 1, 0) >= 4)
        return c;

      // slash
      if (1 + run_length(c, play_row - 1, play_col + 1, -1, 1) + run_length(c, play_row + 1, play_col - 1, 1, -1) >= 4)
        return c;

      // backslash
      if (1 + run_length(c, play_row - 1, play_col - 1, -1, -1) + run_length(c, play_row + 1, play_col + 1, 1, 1) >= 4)
        return c;
    }
    return '0';
  }

  // returns the row the piece landed in
  int make_play(int col)
  {
    int row = rows - 1;
    while (row >= 0)
    {
      if (cell(row, col) == '0')
      {
        cell(row, col) = to_play;
        to_play = opponent(to_play);
        return row;
      }
      --row;
    }
    std::cerr << "Invalid move attempted. :(\n";
    exit(1);
  }

  // populates moves; returns number of moves
  int legal_moves(int moves[MAX_WIDTH]) const
  {
    int count = 0;
    for(int i = 0; i < cols; ++i)
      if (!column_full(i))
        moves[count++] = i;
    return count;
  }

  int random_move() const
  {
    int moves[MAX_WIDTH];
    int count = legal_moves(moves);
    if (count == 0)
      return -1;
    return moves[rand() % count];
  }

  void print(std::ostream &str) const
  {
    str << "to play: " << to_play << '\n';
    for(int i = 0; i < rows; ++i)
    {
      for(int j = 0; j < cols; ++j)
      {
        str << cell(i, j);
      }
      str << '\n';
    }
  }
};

// given the state, player, and move to make, return the probability of a win
// examine all subgames to g_Ply and then do g_Trials random games from each leaf
double evaluate_move(const GameState &game, int move, int depth = 0)
{
  GameState sim(game);
  char me = sim.turn();

  // apply move and check for endgame condition
  int row = sim.make_play(move);
  char res = sim.check_state(row, move);
  if (res == me)
    return 1.0; // win!
  else if (res != '0')
    return 0.0; // lose!

  if (depth < g_Ply)
  {
    // find opponent's best move
    double best_prob = 0.0;
    for(int col = 0; col < sim.width(); ++col)
    {
      if (sim.column_full(col))
        continue;
      double prob = evaluate_move(sim, col, depth + 1);
      if (prob > best_prob)
        best_prob = prob;
    }
    // invert to return _my_ probability of winning
    return 1.0 - best_prob;
  }
  else
  {
    // play randomly from here to the end a bunch of times
    // and return the fraction of trials we win
    int wins = 0;
    for(int trial = 0; trial < g_Trials; ++trial)
    {
      GameState sub(sim);
      for(;;)
      {
        int col = sub.random_move();
        if (col < 0)
          break;
        int row = sub.make_play(col);
        char result = sub.check_state(row, col);
        if (result == me)
          ++wins;
        if (result != '0')
          break;
      }
    }
    return (double)wins / g_Trials;
  }
}

struct ThreadState
{
  pthread_t thread_id;
  const GameState *game;
  int col;
  double prob;
};

void *thread_proc(void *param)
{
  ThreadState *ts = (ThreadState *)param;
  ts->prob = evaluate_move(*(ts->game), ts->col);
  return NULL;
}

int play(const GameState &game)
{
  ThreadState threads[MAX_WIDTH];
  tune_depth(game.width());
  for(int col = 0; col < game.width(); ++col)
  {
    threads[col].thread_id = 0;
    threads[col].game = &game;
    threads[col].col = col;
    threads[col].prob = 0.0;
    if (game.column_full(col))
      continue;
    pthread_create(&threads[col].thread_id, NULL, thread_proc, &threads[col]);
  }

  int best_col = 0;
  for(int col = 0; col < game.width(); ++col)
  {
    if (threads[col].thread_id != 0)
      pthread_join(threads[col].thread_id, NULL);
    fprintf(stderr, "%1.2f ", threads[col].prob);
    if (threads[col].prob > threads[best_col].prob)
      best_col = col;
  }
  fprintf(stderr, "\n");
  return best_col;
}

int main(int argc, char **argv)
{
  srand(time(0));

  if (argc >= 3 && 0 == strcmp(argv[1], "--benchmark"))
  {
    int size = atoi(argv[2]);
    if (size < 4 || size > MAX_WIDTH) {
      std::cerr << "invalid size; use --benchmark N, where 4 <= N <= " << MAX_WIDTH << std::endl;
      return 1;
    }
    GameState blank(size, size, '1');
    play(blank);
    return 0;
  }

  GameState game;
  while(game.read())
  {
    int move = play(game);

    int test = game.test_value();
    if (test >= 0) {
      if (move != test) {
        std::cerr << "test failed: expected " << test << "; actual " << move << std::endl;
        game.print(std::cerr);
      }
    }

    std::cout << move << std::endl;
  }
  return 0;
}

