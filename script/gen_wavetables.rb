BIT_SIZE   = 4096
TABLE_SIZE = 1024

def gen_saw
  step = BIT_SIZE / TABLE_SIZE
  table = []
  (0..(TABLE_SIZE-1)).each do |i|
    table << step * i
  end
  table
end

def gen_sin
  table = []
  (0..(TABLE_SIZE-1)).each do |i|
    table << ((BIT_SIZE-1)/2 * Math.sin(2.0*Math::PI*(i.to_f/TABLE_SIZE.to_f))).to_i + (BIT_SIZE-1)/2
  end
  table
end

def gen_sqr
  table = []
  threshold = TABLE_SIZE / 2
  (0..(TABLE_SIZE-1)).each do |i|
    table << (i < threshold ? (BIT_SIZE-1) : 0)
  end
  table
end

def gen_25per_skewed_tri
  table = []
  threshold = TABLE_SIZE / 4
  (0..(TABLE_SIZE-1)).each do |i|
    if i < threshold
      table << (i.to_f / threshold.to_f * (BIT_SIZE-1)).to_i
    else
      table << (BIT_SIZE-1) - ((i-threshold).to_f/(TABLE_SIZE-threshold).to_f * (BIT_SIZE-1)).to_i
    end
  end
  table
end

def print_table(table)
  puts "{"
    puts "  #{table.join(', ')}"
  puts "},"
end

print_table(gen_saw)
print_table(gen_sin)
print_table(gen_sqr)
print_table(gen_25per_skewed_tri)
