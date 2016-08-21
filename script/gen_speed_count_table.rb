require 'bigdecimal'

base = BigDecimal("6.875")
(0..11).each do |i|
  (0..255).each do |j|
    hz = base * (BigDecimal("2.0") ** ((BigDecimal(i) + BigDecimal(j)/256)/12))
    puts "#{((hz / base) * (2**12)).round},"
  end
end





