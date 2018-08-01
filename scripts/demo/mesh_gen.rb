
require 'chunky_png'

logo = ChunkyPNG::Image.from_file('demo_logo_small.png')

lookup = {        # depth   init temp   conductivity
    4294967295 => [ 0.0   ,     0     ,     0.0 ], # white (nothing)
    589242623  => [ 1.0   ,     0     ,     0.2 ], # black (cool letter)
    4136047359 => [ 1.0   ,   511     ,     0.2 ], # orange (hot letter)
    2197781247 => [ 0.25  ,   120     ,     0.2 ], # green (background construct)
}


LEVELS = 3

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

    @@idc = [0] * LEVELS

    def initialize()
        @exists = false
        @id = -1
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
        "cell ##{id} : L#{level} T#{temperature}  C#{conductivity} F(o:#{@in_faces.join(",")}, i:#{@out_faces.join(",")})"
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

cell_structure = Array.new(logo.width) { Array.new(logo.height) { Array.new(MAX_DEPTH) { Cell.new } } } 
cell_array = Array.new(LEVELS) { [] }
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
                build_face(0, 1, this, right)
                this.right_area = 1
            end
            if up.exists?
                build_face(0, 1, this, up) if up.exists?
                this.up_area = 1
            end
            if forward.exists?
                build_face(0, 1, this, forward)
                this.fwd_area = 1
            end
        end
    end
end

## Build hierarchy

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

                cell_array[level] << coarser[x][y][z]
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
                build_face(level, this.right_area, this, right) if right.exists?
                build_face(level, this.up_area, this, up) if up.exists?
                build_face(level, this.fwd_area, this, forward) if forward.exists?
            end
        end
    end
end

#puts "occuring cell counts per combined cell: #{dbg_pcell_sizes.uniq}"

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

File.open("mesh.amf", "wb+") do |out|
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
