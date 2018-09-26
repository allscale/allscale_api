
require 'chunky_png'

img_fn = ARGV.size>0 ? ARGV[0] : 'demo_logo.png'

logo = ChunkyPNG::Image.from_file(img_fn)

lookup = {        # depth   init temp   conductivity
    4294967295 => [ 0.0   ,      0     ,   0.0     ], # white (nothing)
    589242623  => [ 1.0   ,      0     ,   1.0/6.0 ], # black (cool letter)
    4136047359 => [ 1.0   ,    511     ,   1.0/6.0 ], # orange (hot letter)
    2197781247 => [ 0.5   ,    120     ,   1.0/6.0 ], # green (background construct)
}

LEVELS = ARGV.size>1 ? ARGV[1].to_i : 1
MAX_DEPTH = ARGV.size>2 ? ARGV[2].to_i : 16

COARSEST_LENGTH = 2 ** LEVELS 

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
    attr :id
    attr :level
    attr :temperature
    attr :conductivity
    attr :in_faces
    attr :out_faces

    attr_accessor :child_cells
    attr_accessor :parent_cell
    attr_accessor :vertices

    # store connection area for easier hierarchy building 
    attr_accessor :right_area
    attr_accessor :up_area
    attr_accessor :fwd_area

    @@idc = [0] * LEVELS

    def initialize()
        @exists = false
        @id = -1
    end

    def self.new_parent(lvl)
        ret = Cell.new
        ret.set(lvl, 0 ,0)
        ret
    end

    def exists?
        @id != -1
    end 

    def set(level, init, cond)
        @id = @@idc[level]
        @@idc[level] += 1
        @exists = true
        @level = level
        @temperature = init
        @conductivity = cond

        @out_faces ||= []
        @in_faces ||= []
        @child_cells ||= []
        @parent_cell = nil
        @vertices ||= []

        @right_area = 0.0
        @up_area = 0.0
        @fwd_area = 0.0
    end

    def Cell.num_ids
        @@idc.sum
    end

    def add_out_face(f)
        @out_faces << f
    end

    def add_in_face(f)
        @in_faces << f
    end

    def to_s
        "cell #%010d : L%d T%5.1f C%5.1f" % [ id, level, temperature, conductivity] +
            " F(o:#{@in_faces.join(",").rjust(24)}, i:#{@out_faces.join(",").rjust(24)}),"+
            " children(#{@child_cells.map {|c| c.id}.join(",").rjust(48)})" 
    end

    def to_bin
        # cell format
        # level | temperature | conductivity | in face ids (3x) | out face ids (3x) | vertex ids (8x) | child cells (8x)                            
        # int   | double      | double       | int | int | int  | int | int | int   | 8x int          | 8x int
        def make_output(arr, length)
            arr.map { |e| e.id }.fill(-1, arr.size, length - arr.size)
        end
        in_faces_write = make_output(in_faces, 3)
        out_faces_write = make_output(out_faces, 3)
        vertices_write = make_output(vertices, 8)
        child_cells_write = make_output(child_cells, 8)
        raise "Cell with no child cells!" if level > 0 && !child_cells.any? { |c| c.exists? }
        raise "Cell with no parent cell!\n#{to_s}" if level < LEVELS-1 && !parent_cell
        out_array = ([level, temperature, conductivity] + in_faces_write + out_faces_write + vertices_write + child_cells_write)
        ret = out_array.pack("ldd#{"l"*(3+3+8+8)}")
        rs = ret.size
        raise "Unexpected binary format cell size: #{rs} bytes" unless rs == 108
        return ret
    end
end

class Face
    attr :id
    attr :level
    attr :area
    attr :in_cell
    attr :out_cell

    @@idc = [0] * LEVELS

    def initialize(level, area, in_cell, out_cell)
        @id = @@idc[level]
        @@idc[level] += 1
        @level = level
        @area = area
        @in_cell = in_cell
        @out_cell = out_cell
    end
    
    def Face.num_ids
        @@idc.sum
    end

    def to_s
        "f#{id}"
    end

    def to_bin
        # face format
        # level | area   | in cell | out cell
        # int   | double | int     | int
        [level, area, in_cell.id, out_cell.id].pack("ldll")
    end
end

class Vertex
    attr :id
    attr :position

    @@idc = 0

    def initialize()
        @id = -1
    end
    
    def exists?
        @id != -1
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
    
    def to_bin
        # vertex format
        # x pos  | y pos  | z pos 
        # double | double | double
        position.pack("ddd")
    end
end

$face_array = Array.new(LEVELS) { [] }

def build_face(level, area, cell_in, cell_out)
    face = Face.new(level, area, cell_in, cell_out)
    cell_in.add_out_face(face)
    cell_out.add_in_face(face)
    $face_array[level] << face
end


## Build cells from image and add vertices

def round_to(num, target_divisor)
    num + (num % target_divisor)
end

# expand working area to be divisible by coarsest cell width
WIDTH = round_to(logo.width, COARSEST_LENGTH)
HEIGHT = round_to(logo.height, COARSEST_LENGTH)
DEPTH = round_to(MAX_DEPTH, COARSEST_LENGTH)

cell_structure = Array.new(WIDTH) { Array.new(HEIGHT) { Array.new(DEPTH) { Cell.new } } } 
cell_array = Array.new(LEVELS) { [] }
cell_count = 0

vertex_structure = Array.new(WIDTH+1) { Array.new(HEIGHT+1) { Array.new(DEPTH+1) { Vertex.new } } }
vertex_array = []

WIDTH.times do |x|
    HEIGHT.times do |y|
        next if x >= logo.width || y >= logo.height
        col = logo[x,y]
        raise "unexpected color" unless lookup.include?(col)
        depth, init, cond = lookup[col]

        if(depth > 0)
            num = (MAX_DEPTH*depth).ceil
            start_z = (MAX_DEPTH-num)/2
            num.times { |i|
                z = start_z+i
                cell_structure[x][y][z].set(0, init, cond)
                cell_array[0] << cell_structure[x][y][z]
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

WIDTH.times do |x|
    HEIGHT.times do |y|
        DEPTH.times do |z|
            # connect to right, up and fwd
            this = cell_structure[x][y][z]
            next unless this.exists?

            if x < WIDTH-1
                right = cell_structure[x+1][y][z]
                if right.exists?
                    build_face(0, 1, this, right)
                    this.right_area = 1
                end
            end
            if y < HEIGHT-1
                up = cell_structure[x][y+1][z]
                if up.exists?
                    build_face(0, 1, this, up) if up.exists?
                    this.up_area = 1
                end
            end
            if z < DEPTH-1
                forward = cell_structure[x][y][z+1]
                if forward.exists?
                    build_face(0, 1, this, forward)
                    this.fwd_area = 1
                end
            end
        end
    end
end

## Build hierarchy

dbg_pcell_sizes = []

level_cell_structure = []
level_cell_structure[0] = cell_structure
1.upto(LEVELS-1) do |level|
    factor = 2.0 ** level
    lw = (WIDTH/factor).ceil
    lh = (HEIGHT/factor).ceil
    ld = (DEPTH/factor).ceil
    level_cell_structure[level] = Array.new(lw) { Array.new(lh) { Array.new(ld) { Cell.new } } }
    coarser = level_cell_structure[level]
    finer = level_cell_structure[level-1]

    # build coarser cells
    lw.times do |x|
        lh.times do |y|
            ld.times do |z|
                child_cells = []
                right_child_cells = []
                up_child_cells = []
                fwd_child_cells = []
                REL_IDS.each do |rid|
                    fxid = x*2+rid[0]
                    fyid = y*2+rid[1]
                    fzid = z*2+rid[2]
                    next if fxid >= finer.size || fyid >= finer[fxid].size || fzid >= finer[fxid][fyid].size
                    target = finer[fxid][fyid][fzid]
                    if target.exists?
                        child_cells << target 
                        right_child_cells << target if rid[0]==1
                        up_child_cells << target if rid[1]==1
                        fwd_child_cells << target if rid[2]==1
                    end
                end
                pcount = child_cells.size
                dbg_pcell_sizes << pcount
                next if pcount == 0

                # build cell with avg temp and conductivity
                avg_init = child_cells.reduce(0.0) { |memo, c| memo + c.temperature } / pcount.to_f
                avg_cond = child_cells.reduce(0.0) { |memo, c| memo + c.conductivity } / pcount.to_f
                coarser[x][y][z].set(level, avg_init, avg_cond)

                # aggreagte areas
                coarser[x][y][z].right_area = right_child_cells.reduce(0.0) { |memo, c| memo + c.right_area }
                coarser[x][y][z].up_area = up_child_cells.reduce(0.0) { |memo, c| memo + c.up_area }
                coarser[x][y][z].fwd_area = fwd_child_cells.reduce(0.0) { |memo, c| memo + c.fwd_area }

                # connect hierarchy
                coarser[x][y][z].child_cells = child_cells
                child_cells.each { |cc| cc.parent_cell = coarser[x][y][z] }

                cell_array[level] << coarser[x][y][z]
            end
        end
    end

    # build coarser cell connection faces
    lw.times do |x|
        lh.times do |y|
            ld.times do |z|
                # connect to right, up and fwd
                this = coarser[x][y][z]
                next unless this.exists?
    
                if x < lw-1
                    right = coarser[x+1][y][z]
                    build_face(level, this.right_area, this, right) if right.exists? && this.right_area > 0
                end
                if y < lh-1
                    up = coarser[x][y+1][z]
                    build_face(level, this.up_area, this, up) if up.exists? && this.up_area > 0
                end
                if z < ld-1
                    forward = coarser[x][y][z+1]
                    build_face(level, this.fwd_area, this, forward) if forward.exists? && this.fwd_area > 0
                end
            end
        end
    end
end

## DEBUGGING

# find 100% deep stick of cells
#WIDTH.times do |x|
#    HEIGHT.times do |y|
# x = 16
# y = 16
#         stick = level_cell_structure[0][x][y]
#         #next unless stick.all? {|c| c.exists? }
#         puts "Found #{x}/#{y}:"
#         stick.each_with_index { |c,z| puts "(%2d/%2d/%2d) " % [x,y,z] + c.to_s }
#         exit
#    end
#end

#puts "occuring cell counts per combined cell: #{dbg_pcell_sizes.uniq}"

## /DEBUGGING

puts "levels: #{LEVELS}"
puts "vertices: #{vertex_array.size}"
LEVELS.times { |l|
    puts  "LEVEL %d - %10d Cells %10d Faces" % [l, cell_array[l].size, $face_array[l].size]
}

puts "\nStarting file dump"

# dump obj for inspection

create_obj_file = ARGV.include?("-obj")
if(create_obj_file)
    File.open("level0.obj", "w+") do |out|
        out.puts "mtllib ramp.mtl"
        # dump vertices
        vertex_array.each_with_index { |v,i|
            raise "index error #{i} != #{v.id}" if i != v.id
            out.puts "v #{v.position[0]} #{v.position[1]} #{v.position[2]}"
        }
        # dump cells
        cell_array[0].each do |c|
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
end

# file format
# id -1 = empty

# header:
# 4 bytes   | 4 bytes int | 4 bytes int  | num_vertices * 24 bytes | 4 bytes   
# #A115ca1e | num_levels  | num_vertices | vertices                | #A115ca1e 

# PER LEVEL:
# 4 bytes   | 4 bytes int | 4 bytes int | 4 bytes int | num_cells * 108 bytes | 4 bytes   | num_faces * 20 bytes | 4 bytes   
# #A115ca1e | level       | num_cells   | num_faces   | cells                 | #A115ca1e | faces                | #A115ca1e 

def write_magic_number(outf)
    outf.write([0xA115ca1e].pack("l"))
end

def write_array(outf, arr, name, elemsize)
    fp = outf.pos
    arr.each_with_index { |e,i| 
        written_bytes = outf.write(e.to_bin)
        raise "Unexpected number of bytes written" if written_bytes != elemsize
        raise "Index mismatch" if i != e.id
    }
    puts "#{name} written: #{(outf.pos - fp) / elemsize} (#{(outf.pos - fp)} bytes)"
    write_magic_number(outf)
end

File.open("mesh_#{img_fn[0...-4]}_d#{MAX_DEPTH}_l#{LEVELS}.amf", "wb+") do |out|
    # header
    write_magic_number(out)
    out.write([LEVELS, vertex_array.size].pack("ll"))

    # vertices
    write_array(out, vertex_array, "Vertices", 24)

    LEVELS.times do |l|
        puts "LEVEL #{l}"

        # per-level header
        write_magic_number(out)
        out.write([l, cell_array[l].size, $face_array[l].size].pack("lll"))

        write_array(out, cell_array[l], "Cells", 108)
        write_array(out, $face_array[l], "Faces", 20)
    end
    
end
