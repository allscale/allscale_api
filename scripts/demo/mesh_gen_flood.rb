
require 'chunky_png'
require 'pqueue'

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
MAX_LVL1_CELLS = 6

MAX_FACES = 25
CELL_BYTE_SIZE = 4 + 8 + 8 + 2*MAX_FACES*4 + 8*4 + 8*4

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

    attr_accessor :clusters

    @@idc = [0] * LEVELS

    def initialize()
        @id = -1
        @parent_cell = nil
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
        
        @clusters = [""] * 32

        @out_faces ||= []
        @in_faces ||= []
        @child_cells ||= []
        @vertices ||= []
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

    def connected_cells
        ret = []
        @in_faces.each { |f| ret << f.in_cell }
        @out_faces.each { |f| ret << f.out_cell }
        ret
    end

    def connected_to?(other)
        return @in_faces.any? { |f| f.in_cell == other } || 
               @out_faces.any? { |f| f.out_cell == other }
    end

    def connected_free_cells(level)
        ret = []
        @in_faces.each { |f| ret << f.in_cell unless f.in_cell.clusters[level] != "" }
        @out_faces.each { |f| ret << f.out_cell unless f.out_cell.clusters[level] != "" }
        ret
    end

    def to_s
        "cell #%010d : L%d T%5.1f C%5.1f" % [ id, level, temperature, conductivity] +
            " F(o:#{@in_faces.join(",").rjust(24)}, i:#{@out_faces.join(",").rjust(24)}),"+
            " children(#{@child_cells.map {|c| c.id}.join(",").rjust(48)})" 
    end

    def inspect
        "c#{id}"
    end

    def to_bin
        # cell format
        # level | temperature | conductivity | in face ids (MAX_FACESx) | out face ids (MAX_FACESx) | vertex ids (8x) | child cells (8x)                            
        # int   | double      | double       | MAX_FACESx int           | MAX_FACESx int            | 8x int          | 8x int
        def make_output(arr, length)
            arr.map { |e| e.id }.fill(-1, arr.size, length - arr.size)
        end
        in_faces_write = make_output(in_faces, MAX_FACES)
        out_faces_write = make_output(out_faces, MAX_FACES)
        vertices_write = make_output(vertices, 8)
        child_cells_write = make_output(child_cells, 8)
        raise "Cell with no child cells!" if level > 0 && !child_cells.any? { |c| c.exists? }
        raise "Cell with no parent cell!\n#{to_s}" if level < LEVELS-1 && !parent_cell
        raise "Too many children (#{child_cells.size})" if child_cells.size > 8
        raise "Too many in faces (#{in_faces.size})" if in_faces.size > MAX_FACES
        raise "Too many out faces (#{out_faces.size})" if out_faces.size > MAX_FACES
        child_cells.each { |c| raise "Bad parenting" if c.parent_cell != self }
        child_cells.each { |c| raise "Bad hierarchy" if c.level != level-1 }
        out_array = ([level, temperature, conductivity] + in_faces_write + out_faces_write + vertices_write + child_cells_write)
        ret = out_array.pack("ldd#{"l"*(2*MAX_FACES + 8 + 8)}")
        rs = ret.size
        raise "Unexpected binary format cell size: #{rs} bytes" unless rs == CELL_BYTE_SIZE
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
        raise "Bad face level" if in_cell.level != out_cell.level || in_cell.level != level
        raise "Self face" if in_cell == out_cell
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

    def x
        @position[0]
    end
    def y
        @position[1]
    end
    def z
        @position[2]
    end
    def x=(v)
        @position[0] = v
    end
    def y=(v)
        @position[1] = v
    end
    def z=(v)
        @position[2] = v
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
            num.times do |i|
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
            end
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
                end
            end
            if y < HEIGHT-1
                up = cell_structure[x][y+1][z]
                if up.exists?
                    build_face(0, 1, this, up) if up.exists?
                end
            end
            if z < DEPTH-1
                forward = cell_structure[x][y][z+1]
                if forward.exists?
                    build_face(0, 1, this, forward)
                end
            end
        end
    end
end

## Build hierarchy

NUM_RD_SEEDS = 10

def dist(a,b)
    return Math.sqrt((a.vertices[0].x - b.vertices[0].x)**2 +
        (a.vertices[0].y - b.vertices[0].y)**2 +
        (a.vertices[0].z - b.vertices[0].z)**2)
end

class Cluster
    attr_accessor :level
    attr_accessor :cells
    attr_accessor :child_clusters
    attr_accessor :parent_cluster

    def initialize()
        @level = -1
        @cells = []
        @child_clusters = []
        @parent_cluster = nil
    end
end

def cluster(input_cells, level, containing_cluster_id)
    if input_cells.size <= MAX_LVL1_CELLS
        return 0, input_cells
    end

    # Pick randomly
    sample_cells = input_cells.sample(10)
    sample_pairs = sample_cells.permutation(2).to_a
    sample_pairs = sample_pairs.sort_by { |a,b| -dist(a,b) }
    seedA, seedB = sample_pairs[0]

    cid0 = containing_cluster_id + "0"
    cid1 = containing_cluster_id + "1"
    seedA.clusters[level] = cid0
    seedB.clusters[level] = cid1

    pqA = PQueue.new([seedA]) { |a,b| dist(a,seedB) > dist(b,seedB) }
    pqB = PQueue.new([seedB]) { |a,b| dist(a,seedA) > dist(b,seedA) }

    clusterA = [seedA]
    clusterB = [seedB]

    while !(pqA.empty? && pqB.empty?)
        pq, cluster_id, cur_cluster = pqA, cid0, clusterA
        pq, cluster_id, cur_cluster = pqB, cid1, clusterB if (clusterB.size < clusterA.size && !pqB.empty?) || pqA.empty?

        add_cell = pq.pop
        fc = add_cell.connected_free_cells(level)
        if level > 0
            fc.reject! { |c| c.clusters[level-1] != containing_cluster_id }
        end
        fc.each { |c| c.clusters[level] = cluster_id }
        pq.merge!(fc)
        cur_cluster.concat(fc)
    end

    lvlA, clusters_fromA = cluster(clusterA, level+1, cid0)
    lvlB, clusters_fromB = cluster(clusterB, level+1, cid1)
    while lvlA < lvlB
        clusters_fromA = [ clusters_fromA ] 
        lvlA += 1
    end
    while lvlB < lvlA
        clusters_fromB = [ clusters_fromB ] 
        lvlB += 1
    end

    return lvlA+1, [clusters_fromA, clusters_fromB]
end

$cluster_lvls = Array.new(32) { [] }

def build_cluster_hierarchy(clusters)
    if !clusters.any? { |x| x.kind_of?(Array) }
        leaf = Cluster.new
        leaf.cells = clusters
        leaf.cells.each
        leaf.level = 0
        $cluster_lvls[0] << leaf
        return leaf
    end
    ret = Cluster.new
    lvl = 0
    clusters.each do |c|
        sub = build_cluster_hierarchy(c)
        lvl = [sub.level+1, lvl].max
        sub.parent_cluster = ret
        ret.child_clusters << sub
    end
    ret.level = lvl
    $cluster_lvls[lvl] << ret
    return ret
end

puts "Start clustering"
levels, clusters = cluster(cell_array[0], 0, "")
puts "Clustered to #{levels} levels"
clus_hierarchy = build_cluster_hierarchy(clusters)
puts "Cluster hierarchy levels: #{clus_hierarchy.level + 1}"

#$cluster_lvls.each_with_index do |lvl, idx| 
#    puts "#{idx}: #{lvl.size}" unless lvl.size == 0
#end

puts "Building cell hierarchy"
(LEVELS-1).times do |level|
    $cluster_lvls[level].each do |current_cluster|
        # build a cell for each cluster
        new_parent_cell = Cell.new
        child_cells = current_cluster.cells
        # init parent cell with avg temp and conductivity
        avg_init = child_cells.reduce(0.0) { |memo, c| memo + c.temperature } / child_cells.size.to_f
        avg_cond = child_cells.reduce(0.0) { |memo, c| memo + c.conductivity } / child_cells.size.to_f
        new_parent_cell.set(level+1, avg_init, avg_cond)
        # connect child cells to parent
        raise "Missing child cells" if child_cells.empty?
        child_cells.each do |c|
            raise "Already has a parent" if c.parent_cell
            c.parent_cell = new_parent_cell
        end
        raise "Already has children" unless new_parent_cell.child_cells.empty?
        new_parent_cell.child_cells = child_cells
        # add new cell to cell array for this level, and to parent cluster
        cell_array[level+1] << new_parent_cell
        current_cluster.parent_cluster.cells << new_parent_cell
    end
end

puts "Building cell faces at coarser levels"

max_faces = [0] * 32

1.upto(LEVELS-1) do |level|
    cell_array[level].each do |cell|
        child_cells = cell.child_cells
        
        connections = Hash.new() { |h,k| h[k] = 0 }
        child_cells.each { |child|
            child.in_faces.each { |face|
                connected_parent = face.in_cell.parent_cell
                raise "parentless cell" unless connected_parent   
                connections[connected_parent] += face.area if connected_parent != cell
            }
            child.out_faces.each { |face|
                connected_parent = face.out_cell.parent_cell  
                raise "parentless cell" unless connected_parent              
                connections[connected_parent] += face.area if connected_parent != cell 
            }
        }
        #print "#{cell.inspect.rjust(4)} : "
        #p connections
        
        max_faces[level] = [max_faces[level], connections.size].max

        connections.each do |connected_cell, area|
            # don't enter connections twice
            next if cell.connected_to?(connected_cell)
            # order cells
            sorted_cells = [cell, connected_cell].sort_by { |c| c.id }
            # build face
            build_face(level, area, sorted_cells[0], sorted_cells[1])
        end
    end
end

####### Debugging
if false
    def flatten_outer(arr)
        return [arr] unless arr.kind_of?(Array)
        if arr.any? { |x| x.kind_of?(Array) }
            ret = []
            arr.each do |sub|
                ret = ret + flatten_outer(sub)
            end
            return ret
        end
        return [arr]
    end

    flatc = flatten_outer(clusters)
    p flatc.size
    puts "#{flatc.inject(0) { |s,v| s += v.size }} == #{cell_array[0].size}"

    File.open("lvl1.obj", "w+") do |out|
        out.puts "mtllib clusters.mtl"
        # dump vertices
        vertex_array.each_with_index { |v,i|
            raise "index error #{i} != #{v.id}" if i != v.id
            out.puts "v #{v.position[0]} #{v.position[1]} #{v.position[2]}"
        }
        flatc.each_with_index do |cells, mtl_id|
            out.puts "usemtl k#{mtl_id%15}"
            cells.each do |c|
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

    p clusters.size()
    p clusters.flatten(1).size()
end
####### /DEBUGGING

puts "levels: #{LEVELS}"
puts "vertices: #{vertex_array.size}"
LEVELS.times { |l|
    puts  "LEVEL %d - %10d Cells %10d Faces (%4.1f faces per cell, %2d maximum)" % 
          [l, cell_array[l].size, $face_array[l].size, $face_array[l].size.to_f / cell_array[l].size, max_faces[l] ]
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
# 4 bytes   | 4 bytes int | 4 bytes int | 4 bytes int | num_cells * CELL_BYTE_SIZE bytes | 4 bytes   | num_faces * 20 bytes | 4 bytes   
# #A115ca1e | level       | num_cells   | num_faces   | cells                            | #A115ca1e | faces                | #A115ca1e 

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

File.open("mesh_flood_#{img_fn[0...-4]}_d#{MAX_DEPTH}_l#{LEVELS}.amf", "wb+") do |out|
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

        write_array(out, cell_array[l], "Cells", CELL_BYTE_SIZE)
        write_array(out, $face_array[l], "Faces", 20)
    end
    
end
