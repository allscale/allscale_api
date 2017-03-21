/**
 * A utility to generate meshes for bars.
 */

struct Vertex {};
struct Edge : public allscale::api::user::data::edge<Vertex,Vertex> {};
struct Refine : public allscale::api::user::data::hierarchy<Vertex,Vertex> {};


template<unsigned levels, unsigned partitionDepth = 5>
using BarMesh = allscale::api::user::data::Mesh<
		allscale::api::user::data::nodes<Vertex>,
		allscale::api::user::data::edges<Edge>,
		allscale::api::user::data::hierarchies<Refine>,
		levels,
		partitionDepth
		>;

template<unsigned levels>
using BarMeshBuilder = allscale::api::user::data::MeshBuilder<allscale::api::user::data::nodes<Vertex>,allscale::api::user::data::edges<Edge>,allscale::api::user::data::hierarchies<Refine>,levels>;

namespace detail {

	template<unsigned level, unsigned levels>
	void createLevelVertices(BarMeshBuilder<levels>& builder, unsigned numVertices) {

		// insert vertices
		auto vertices = builder.template create<Vertex,level>(numVertices);
		assert_eq(numVertices,vertices.size());

		// connect vertices through edges
		for(unsigned i = 1; i < numVertices; ++i) {
			builder.template link<Edge>(vertices[i],vertices[i-1]);
		}

		for(unsigned i = 0; i < numVertices - 1; ++i) {
			builder.template link<Edge>(vertices[i],vertices[i+1]);
		}

	}


	template<unsigned level>
	struct BarMeshLevelBuilder {

		template<unsigned levels>
		void operator()(BarMeshBuilder<levels>& builder, unsigned numVertices) {
			createLevelVertices<level>(builder,numVertices);
			BarMeshLevelBuilder<level-1>()(builder,numVertices * 2);

			// connect hierarchical edges
			for(unsigned i = 0; i<numVertices; ++i) {
				builder.template link<Refine>(
						allscale::api::user::data::NodeRef<Vertex,level>(i),
						allscale::api::user::data::NodeRef<Vertex,level-1>(2*i)
				);

				builder.template link<Refine>(
						allscale::api::user::data::NodeRef<Vertex,level>(i),
						allscale::api::user::data::NodeRef<Vertex,level-1>(2*i+1)
				);
			}
		}

	};

	template<>
	struct BarMeshLevelBuilder<0> {

		template<unsigned levels>
		void operator()(BarMeshBuilder<levels>& builder, unsigned numVertices) {
			createLevelVertices<0>(builder,numVertices);
		}

	};


}


template<unsigned levels = 1, unsigned partitionDepth = 5>
BarMesh<levels,partitionDepth> createBarMesh(unsigned length) {
	using builder_type = BarMeshBuilder<levels>;

	// create a mesh builder
	builder_type builder;

	// fill mesh on all levels
	detail::BarMeshLevelBuilder<levels-1>()(builder,length);

	// finish the mesh
	return std::move(builder).template build<partitionDepth>();

}
