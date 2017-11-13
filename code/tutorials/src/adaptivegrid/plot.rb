#Usage: ruby plot.rb wave_result.csv

require "gnuplot"
require "csv"
require "parallel"

data = CSV.read(ARGV[0])

work = (1..(data.size - 1)).to_a

Parallel.each(work) { | i |

    x = Array.new
    y = Array.new
    z = Array.new

    data[i].each_index do | index |
        if index == 0
            next
        end
        element = data[i][index]

        pos = data[0][index].split(":")
        if x[-1] != pos[0]
            x.push nil
            y.push nil
            z.push nil
        end
        x.push pos[0]
        y.push pos[1]
        z.push element
        #puts "[#{pos[0]}][#{pos[1]}] = #{element}"
    end



    Gnuplot.open do |gp|
        Gnuplot::SPlot.new( gp ) do |plot|
            plot.grid
            plot.zrange "[-0.5:1]"
            plot.cbrange "[-0.5:1]"
            plot.palette "defined (0 \"black\", 1 \"cyan\")"
            plot.ylabel "y"
            plot.xlabel "x"
            plot.zlabel "amp"

            plot.terminal "png"
            plot.output File.expand_path("../animation_#{i}.png", __FILE__)

            plot.data = [
            Gnuplot::DataSet.new( [x, y, z] ) do |ds|
                ds.with = "pm3d"
            end
            ]
        end
    end
}