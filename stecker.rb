require "json"

$stdin.sync = true
$stdout.sync = true
IO.popen('./stecker-engine', 'a+') do |f|
  f.sync = true
  loop do
    json = JSON.parse gets
    if json['winner']
      f.print '0000'
      break
    end
    rows = json['board'].size
    cols = json['board'][0].size
    f.printf "%02d", rows
    f.printf "%02d", cols
    f.print json['board'].map { |row|
      row.map { |cell|
        case cell
        when 0
          '.'
        when json['currentPlayer']
          'X'
        else
          'O'
        end
      }.join
    }.join
    puts f.read(2).to_i.to_s
  end
end
