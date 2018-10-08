

require 'chunky_png'

ramp = ChunkyPNG::Image.from_file('color_ramp.png')

def r(p)
    ChunkyPNG::Color.r(p)/256.0
end
def g(p)
    ChunkyPNG::Color.g(p)/256.0
end
def b(p)
    ChunkyPNG::Color.b(p)/256.0
end

ramp.height.times do |y|
    px = ramp.pixels[y]
    puts "newmtl r#{ramp.height-y-1}"
    puts "Kd #{r(px)} #{g(px)} #{b(px)}"
    puts
end