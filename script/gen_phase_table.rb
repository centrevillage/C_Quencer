TABLE_SIZE = 1024

def gen_tri
  table = []
  step = 2
  (0..255).each do |i|
    table << step * i
  end
  (0..255).each do |i|
    table << (512 - step * i)
  end
  (0..255).each do |i|
    table << (TABLE_SIZE - step * i)
  end
  (0..255).each do |i|
    table << (TABLE_SIZE/2 + step * i)
  end
  table
end

def print_table(table)
  puts "{"
    puts "  #{table.join(', ')}"
  puts "},"
end

print_table(gen_tri)
