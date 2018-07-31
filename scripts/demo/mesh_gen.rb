
require 'chunky_png'

logo = ChunkyPNG::Image.from_file('demo_logo_small.png')

lookup = {        # depth   init temp   conductivity
    4294967295 => [ 0.0   ,     0     ,     0.0 ], # white (nothing)
    589242623  => [ 1.0   ,     0     ,     0.2 ], # black (cool letter)
    4136047359 => [ 1.0   ,   511     ,     0.2 ], # orange (hot letter)
    2197781247 => [ 0.25  ,   120     ,     0.2 ], # green (background construct)
}

MAX_DEPTH = 16


class Cell
    attr :exists
    attr :id
    attr :level
    attr :temperature
    attr :conductivity
    attr :in_faces
    attr :out_faces

    # store connection area for easier hierarchy building 
    attr :right_area
    attr :up_area
    attr :fwd_area

    @@idc = 0

    def initialize()
        @exists = false
    end

    alias :exists? :exists

    def set(level, init, cond)
        @id = @@idc
        @@idc += 1
        @exists = true
        @level = level
        @temperature = init
        @conductivity = cond
        @out_faces ||= []
        @in_faces ||= []
    end

    def Cell.num_ids
        @@idc
    end

    def add_out_face(f)
        @out_faces << f
    end

    def add_in_face(f)
        @in_faces << f
    end

    def to_s
        "cell ##{id} : L#{level} T#{temperature}  C#{conductivity} F(o:#{@in_faces.join(",")}, i:#{@out_faces.join(",")})"
    end
end

class Face
    attr :id
    attr :area
    attr :in_cell
    attr :out_cell

    @@idc = 0

    def initialize(area, in_cell, out_cell)
        @id = @@idc
        @@idc += 1
        @area = area
        @in_cell = in_cell
        @out_cell = out_cell
    end
    
    def Face.num_ids
        @@idc
    end

    def to_s
        "f#{id}"
    end
end

$face_array = []

def build_face(area, cell_in, cell_out)
    face = Face.new(area, cell_in, cell_out)
    cell_in.add_out_face(face)
    cell_out.add_in_face(face)
    $face_array << face
end


## Build cells from image

cell_structure = Array.new(logo.width) { Array.new(logo.height) { Array.new(MAX_DEPTH) { Cell.new } } } 
cell_array = []
cell_count = 0

logo.width.times do |x|
    logo.height.times do |y|
        col = logo[x,y]
        raise "unexpected color" unless lookup.include?(col)
        depth, init, cond = lookup[col]

        if(depth > 0)
            num = (MAX_DEPTH*depth).ceil
            start_z = (MAX_DEPTH-num)/2
            num.times { |i|
                cell_structure[x][y][start_z+i].set(0, init, cond)
                cell_array << cell_structure[x][y][start_z+i]
                cell_count += 1
            }
        end
    end
end

## Build faces connecting cells

(logo.width-1).times do |x|
    (logo.height-1).times do |y|
        (MAX_DEPTH-1).times do |z|
            # connect to right, up and fwd
            this = cell_structure[x][y][z]
            next unless this.exists?

            right = cell_structure[x+1][y][z]
            up = cell_structure[x][y+1][z]
            forward = cell_structure[x][y][z+1]
            build_face(1, this, right, :right) if right.exists?
            build_face(1, this, up, :up) if up.exists?
            build_face(1, this, forward, :fwd) if forward.exists?
        end
    end
end

## Build hierarchy

LEVELS = 3

dbg_pcell_sizes = []

level_cell_structure = []
level_cell_structure[0] = cell_structure
1.upto(LEVELS-1) do |level|
    factor = 2 ** level
    lw = logo.width/factor
    lh = logo.height/factor
    ld = MAX_DEPTH/factor
    level_cell_structure[level] = Array.new(lw) { Array.new(lh) { Array.new(ld) { Cell.new } } }
    coarser = level_cell_structure[level]
    finer = level_cell_structure[level-1]

    # build coarser cells
    lw.times do |x|
        lh.times do |y|
            ld.times do |z|
                relative_ids = [ [0,0,0], [0,0,1], [0,1,0], [0,1,1], [1,0,0], [1,0,1], [1,1,0], [1,1,1] ]
                parent_cells = []
                relative_ids.each do |rid|
                    target = finer[x*2+rid[0]][y*2+rid[1]][z*2+rid[2]]
                    parent_cells << target if target.exists?
                end
                pcount = parent_cells.size
                dbg_pcell_sizes << pcount
                next if pcount == 0
                avg_init = parent_cells.reduce { |memo, c| memo + c.temperature } / pcount.to_f
                avg_cond = parent_cells.reduce { |memo, c| memo + c.conductivity } / pcount.to_f
                coarser[x][y][z].set(level, avg_init, avg_cond)
            end
        end
    end

    # build coarser cell connection faces
    (lw-1).times do |x|
        (lh-1).times do |y|
            (ld-1).times do |z|
                # connect to right, up and fwd
                this = coarser[x][y][z]
                next unless this.exists?
    
                right = coarser[x+1][y][z]
                up = coarser[x][y+1][z]
                forward = coarser[x][y][z+1]
                build_face(this.right_area, this, right, :right) if right.exists?
                build_face(this.up_area, this, up, :up) if up.exists?
                build_face(this.fwd_area, this, forward, :fwd) if forward.exists?
            end
        end
    end

end 

puts "occuring cell counts per combined cell: #{dbg_pcell_sizes.uniq}"

puts "cell_count: #{cell_count} / num cell ids: #{Cell.num_ids}"
puts "num face ids: #{Face.num_ids}"
puts cell_array[0].to_s
puts cell_array[cell_count/4].to_s
puts cell_array[cell_count-1].to_s

#puts logo.pixels.uniq.map { |c| "#{c} : " + ChunkyPNG::Color.to_truecolor_bytes(c).join(", ") }.join("\n")