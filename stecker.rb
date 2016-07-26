require "json"

$stdin.sync = true
$stdout.sync = true

# true == win; false == lose; nil == game in progress
def check_state(game, me)
  board = game['board']
  h = board.size
  w = board[0].size
  h.times do |row|
    w.times do |col|
      p = board[row][col]
      if p != 0
        # horizontal
        return p == me if col <= w - 4 && board[row][col + 1] == p && board[row][col + 2] == p && board[row][col + 3] == p
        # vertical
        return p == me if row <= h - 4 && board[row + 1][col] == p && board[row + 2][col] == p && board[row + 3][col] == p
        # slash
        return p == me if col >= 3 && row <= h - 4 && board[row + 1][col - 1] == p && board[row + 2][col - 2] == p && board[row + 3][col - 3] == p
        # backslash
        return p == me if col <= w - 4 && row <= h - 4 && board[row + 1][col + 1] == p && board[row + 2][col + 2] == p && board[row + 3][col + 3] == p
      end
    end
  end
  nil
end

def make_play(game, col)
  row = game['board'].size - 1
  while row >= 0
    if game['board'][row][col] == 0
      game['board'][row][col] = game['currentPlayer']
      game['currentPlayer'] = (game['currentPlayer'] == 1) ? 2 : 1
      return col
    end
    row -= 1
  end
  raise 'invalid play attempted :('
end

def make_random_play(game)
  col = game['board'][0].each_with_index.select{ |pair| pair[0] == 0 }.map { |pair| pair[1] }.sample
  make_play(game, col) if col
end

def run_simulations(game_state, iterations)
  width = game_state['board'][0].size
  wins = [0] * width
  width.times do |col|
    next if game_state['board'][0][col] != 0  # column is full
    prime = true
    iterations.times do
      sim = JSON.parse(JSON.generate(game_state)) # blah deep clone
      make_play(sim, col)
      loop do
        result = check_state(sim, game_state['currentPlayer'])
        wins[col] += 1 if result
        break unless result.nil?
        break unless make_random_play(sim)
        prime = false
      end
      if prime # short-circuit instant-win
        wins[col] = iterations
        break
      end
    end
  end
  wins
end

loop do
  game_state = JSON.parse(gets)
  break if game_state["winner"]

  size = game_state['board'].size * game_state['board'][0].size
  iters = 5_000_000 / (size * size)
  STDERR.puts "using #{iters} iterations"

  wins = run_simulations(game_state, iters)
  STDERR.puts wins.inspect
  puts wins.each_with_index.max[1]
end