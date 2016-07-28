#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <string>

#define MAX_WIDTH 16
#define MAX_BOARD_SIZE (MAX_WIDTH * MAX_WIDTH)

#define PLY 3
#define TRIALS 100

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
// examine all subgames to PLY and then do TRIALS random games from each leaf
int evaluate_move(const GameState &game, int move, int depth = 0)
{
  GameState sim(game);
  char me = sim.turn();
  int row = sim.make_play(move);
  char res = sim.check_state(row, move);
  if (res == me)
    return 100; // win!
  else if (res != '0')
    return 0;   // lose!

  if (depth < PLY)
  {
    int best_prob = 0;
    for(int col = 0; col < sim.width(); ++col)
    {
      if (sim.column_full(col))
        continue;
      // negating probabilities from the opponent's POV
      // FIXME my brain is broke
      int prob = 100 - evaluate_move(sim, col, depth + 1);
      if (prob > best_prob)
        best_prob = prob;
    }
    return best_prob;
  }
  else
  {
    return 50; // TODO the Monte Carlo thang
  }

}

int play(const GameState &game)
{
  int best_col = -1, best_prob = -1;
  for(int col = 0; col < game.width(); ++col)
  {
    if (game.column_full(col))
      continue;
    int prob = evaluate_move(game, col);
    if (prob > best_prob)
    {
      best_prob = prob;
      best_col = col;
    }
  }
  return best_col;
}

int main(int argc, char **argv)
{
  srand(time(0));
  GameState game;
  while(game.read())
  {
    int move = play(game);

    int test = game.test_value();
    if (test >= 0) {
      if (move == test) {
        std::cerr << ".";
      } else {
        std::cerr << "\ntest failed: expected " << test << "; actual " << move << std::endl;
        game.print(std::cerr);
      }
    }

    std::cout << move << std::endl;
  }
  return 0;
}

