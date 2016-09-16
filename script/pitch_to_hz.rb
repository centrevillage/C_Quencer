require 'bigdecimal'

base = 6.875
(3..122).each do |i|
  hz = base * (BigDecimal("2.0") ** (BigDecimal(i)/12))
  cycle_per_tick = (BigDecimal("1000000")/hz) * 8 / 16
  puts cycle_per_tick.round
  #puts "#{(hz.to_f/1000000*16*1024)},"
end



