require "json"

$stdin.sync = true
$stdout.sync = true
IO.popen('./stecker-engine', 'a+') do |f|
  f.sync = true
  loop do
    json = JSON.parse gets
    if json['winner']
      f.puts "0"
      break
    end
    f.puts json['currentPlayer']
    rows = json['board'].size
    cols = json['board'][0].size
    f.puts rows
    f.puts cols
    f.puts json['board'].map { |row| row.map(&:to_s).join }.join("\n")
    print f.readline
  end
end
