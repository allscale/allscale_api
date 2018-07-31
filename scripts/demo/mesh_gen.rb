
require 'chunky_png'

logo = ChunkyPNG::Image.from_file('demo_logo_small.png')

lookup = {        # depth   init temp   conductivity
    4294967295 => [ 0.0   ,     0     ,     0.0 ], # white (nothing)
    589242623  => [ 1.0   ,     0     ,     0.2 ], # black (cool letter)
    4136047359 => [ 1.0   ,   511     ,     0.2 ], # orange (hot letter)
    2197781247 => [ 0.25  ,   120     ,     0.2 ], # green (background construct)
}

MAX_DEPTH = 16

def gen_ids()
    max = 8
    ret_arr = []
    ret_hash = {}
    max.times { |i| 
        cur = [i[2], i[1], i[0]]
        ret_arr << cur
        ret_hash[cur] = i
    }
    return ret_arr, ret_hash
end

REL_IDS, REL_HASH = gen_ids()

class Cell
    attr :exists
    alias :exists? :exists

    attr :id
    attr :level
    attr :temperature
    attr :conductivity
    attr :in_faces
    attr :out_faces

    attr_accessor :child_cells
    attr_accessor :vertices

    # store connection area for easier hierarchy building 
    attr_accessor :right_area
    attr_accessor :up_area
    attr_accessor :fwd_area

    @@idc = 0

    def initialize()
        @exists = false
    end

    def set(level, init, cond)
        @id = @@idc
        @@idc += 1
        @exists = true
        @level = level
        @temperature = init
        @conductivity = cond

        @out_faces ||= []
        @in_faces ||= []
        @child_cells ||= []
        @vertices ||= []

        @right_area = 0.0
        @up_area = 0.0
        @fwd_area = 0.0
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

class Vertex
    attr :exists
    alias :exists? :exists

    attr :id
    attr :position

    @@idc = 0

    def initialize()
        @exists = false
    end

    def Vertex.num_ids
        @@idc
    end

    def enable(pos)
        return true if exists?
        @exists = true
        @id = @@idc
        @@idc += 1
        @position = pos
        return false
    end

    def to_s
        "v#{id}"
    end
end

$face_array = []

def build_face(area, cell_in, cell_out)
    face = Face.new(area, cell_in, cell_out)
    cell_in.add_out_face(face)
    cell_out.add_in_face(face)
    $face_array << face
end


## Build cells from image and add vertices

cell_structure = Array.new(logo.width) { Array.new(logo.height) { Array.new(MAX_DEPTH) { Cell.new } } } 
cell_array = []
cell_count = 0

vertex_structure = Array.new(logo.width+1) { Array.new(logo.height+1) { Array.new(MAX_DEPTH+1) { Vertex.new } } }
vertex_array = []

logo.width.times do |x|
    logo.height.times do |y|
        col = logo[x,y]
        raise "unexpected color" unless lookup.include?(col)
        depth, init, cond = lookup[col]

        if(depth > 0)
            num = (MAX_DEPTH*depth).ceil
            start_z = (MAX_DEPTH-num)/2
            num.times { |i|
                z = start_z+i
                cell_structure[x][y][z].set(0, init, cond)
                cell_array << cell_structure[x][y][z]
                cell_count += 1

                REL_IDS.each do |rid|
                    vx = x+rid[0]
                    vy = y+rid[1]
                    vz = z+rid[2]
                    already_existed = vertex_structure[vx][vy][vz].enable([vx,vy,vz])
                    cell_structure[x][y][z].vertices << vertex_structure[vx][vy][vz]
                    vertex_array << vertex_structure[vx][vy][vz] unless already_existed
                end
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
            if right.exists?
                build_face(1, this, right)
                this.right_area = 1
            end
            if up.exists?
                build_face(1, this, up) if up.exists?
                this.up_area = 1
            end
            if forward.exists?
                build_face(1, this, forward)
                this.fwd_area = 1
            end
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
                child_cells = []
                REL_IDS.each do |rid|
                    target = finer[x*2+rid[0]][y*2+rid[1]][z*2+rid[2]]
                    child_cells << target if target.exists?
                end
                pcount = child_cells.size
                dbg_pcell_sizes << pcount
                next if pcount == 0

                # build cell with avg temp and conductivity
                avg_init = child_cells.reduce(0.0) { |memo, c| memo + c.temperature } / pcount.to_f
                avg_cond = child_cells.reduce(0.0) { |memo, c| memo + c.conductivity } / pcount.to_f
                coarser[x][y][z].set(level, avg_init, avg_cond)

                # aggreagte areas
                coarser[x][y][z].right_area = child_cells.reduce(0.0) { |memo, c| memo + c.right_area }
                coarser[x][y][z].up_area = child_cells.reduce(0.0) { |memo, c| memo + c.up_area }
                coarser[x][y][z].fwd_area = child_cells.reduce(0.0) { |memo, c| memo + c.fwd_area }

                # connect hierarchy
                coarser[x][y][z].child_cells = child_cells
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
                build_face(this.right_area, this, right) if right.exists?
                build_face(this.up_area, this, up) if up.exists?
                build_face(this.fwd_area, this, forward) if forward.exists?
            end
        end
    end
end

puts "occuring cell counts per combined cell: #{dbg_pcell_sizes.uniq}"

puts "cell_count: #{cell_count} / num cell ids: #{Cell.num_ids}"
puts "num face ids: #{Face.num_ids}"
puts "num vtx ids: #{Vertex.num_ids} / vertex_array.length: #{vertex_array.length}"
puts cell_array[0].to_s
puts cell_array[cell_count/4].to_s
puts cell_array[cell_count-1].to_s

vertex_array.sort_by! { |v| v.id }

File.open("level0.obj", "w+") do |out|
    out.puts "mtllib ramp.mtl"
    # dump vertices
    vertex_array.each_with_index { |v,i|
        raise "index error #{i} != #{v.id}" if i != v.id
        out.puts "v #{v.position[0]} #{v.position[1]} #{v.position[2]}"
    }
    # dump cells
    cell_array.each do |c|
        out.puts "usemtl r#{c.temperature.to_i}"
        faces = [ [[0,0,0], [0,0,1], [0,1,1], [0,1,0]] ,
                  [[0,0,0], [1,0,0], [1,0,1], [0,0,1]] ,
                  [[0,0,0], [1,0,0], [1,1,0], [0,1,0]] ,
                  [[1,0,0], [1,0,1], [1,1,1], [1,1,0]] ,
                  [[0,0,1], [1,0,1], [1,1,1], [0,1,1]] ,
                  [[0,1,0], [1,1,0], [1,1,1], [0,1,1]] ]
        faces.each do |f|
            out.puts "f #{f.map { |vid| c.vertices[REL_HASH[vid]].id+1 }.join(" ")}"
        end
    end
end

#puts logo.pixels.uniq.map { |c| "#{c} : " + ChunkyPNG::Color.to_truecolor_bytes(c).join(", ") }.join("\n")