#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <string>

#define MAX_WIDTH 16
#define MAX_BOARD_SIZE (MAX_WIDTH * MAX_WIDTH)

class GameState
{
  int rows, cols;
  char to_play;
  char board[MAX_BOARD_SIZE];

  int run_length(char c, int row, int col, int dr, int dc)
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
  GameState() : rows(0), cols(0), to_play('0') {}
  explicit GameState(const GameState& rhs) :
    rows(rhs.rows), cols(rhs.cols), to_play(rhs.to_play)
  {
    memcpy(board, rhs.board, rows * cols);
  }

  inline int width() { return cols; }
  inline int height() { return rows; }
  inline char turn() { return to_play; }

  // protocol (newline separated)
  // player # 1 or 2; 0 means exit (game over)
  // height
  // width
  // board (`height` lines of width `width` containing '0', '1', and '2')
  // the column to play in is returned
  bool read()
  {
    std::string line;
    std::getline(std::cin, line);
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
        std::cerr << "Invalid line length" << std::endl;
        exit(1);
      }
      memcpy(row_data, line.c_str(), cols);
      row_data += cols;
    }

    return true;
  }

  inline char &cell(int row, int col)
  {
    return board[row * cols + col];
  }

  inline bool column_full(int col)
  {
    return cell(0, col) != '0';
  }

  // test whether the piece just played forms a win
  // return the player name ('1' or '2') or '0' otherwise
  char check_state(int play_row, int play_col)
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
        to_play = (to_play == '1') ? '2' : '1';
        return row;
      }
      --row;
    }
    std::cerr << "Invalid move attempted. :(\n";
    exit(1);
  }

  // populates moves; returns number of moves
  int legal_moves(int moves[MAX_WIDTH])
  {
    int count = 0;
    for(int i = 0; i < cols; ++i)
      if (!column_full(i))
        moves[count++] = i;
    return count;
  }

  int random_move()
  {
    int moves[MAX_WIDTH];
    int count = legal_moves(moves);
    if (count == 0)
      return -1;
    return moves[rand() % count];
  }
};

// test heuristic: play a winning move if there is one; otherwise play randomly
void play(GameState &game)
{
  int moves[MAX_WIDTH];
  char me = game.turn();
  int n = game.legal_moves(moves);
  for(int i = 0; i < n; ++i)
  {
    GameState sim(game);
    int row = sim.make_play(moves[i]);
    if (me == sim.check_state(row, moves[i]))
    {
      std::cout << moves[i] << std::endl;
      return;
    }
  }
  std::cout << game.random_move() << std::endl;
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

