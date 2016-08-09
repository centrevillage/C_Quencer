base = 6.875
(3..122).each do |i|
  hz = base * (2.0 ** (i.to_f/12))
  puts "#{(hz/1000000*16*1024)},"
end



