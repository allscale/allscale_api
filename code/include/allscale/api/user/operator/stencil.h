#pragma once

#include <array>
#include <vector>

#include "allscale/api/user/data/grid.h"
#include "allscale/api/user/data/vector.h"

#include "allscale/api/user/operator/pfor.h"
#include "allscale/api/user/operator/async.h"
#include "allscale/api/user/operator/internal/operation_reference.h"

namespace allscale {
namespace api {
namespace user {


	// ---------------------------------------------------------------------------------------------
	//									    Declarations
	// ---------------------------------------------------------------------------------------------


	template<std::size_t dims>
	using Coordinate = data::Vector<std::int64_t,dims>;

	namespace implementation {

		class coarse_grained_iterative;

		class fine_grained_iterative;

		class recursive;

	}


	template<typename Impl>
	class stencil_reference;

	template<typename Impl = implementation::fine_grained_iterative, typename Container, typename Update>
	stencil_reference<Impl> stencil(Container& res, int steps, const Update& update);



	// ---------------------------------------------------------------------------------------------
	//									    Definitions
	// ---------------------------------------------------------------------------------------------


	template<typename Impl>
	class stencil_reference : public internal::operation_reference {

	public:

		// inherit all constructors
		using operation_reference::operation_reference;

	};


	template<typename Impl, typename Container, typename Update>
	stencil_reference<Impl> stencil(Container& a, int steps, const Update& update) {

		// forward everything to the implementation
		return Impl().process(a,steps,update);

	}

	namespace implementation {

		// -- Iterative Stencil Implementation ---------------------------------------------------------

		struct coarse_grained_iterative {

			template<typename Container, typename Update>
			stencil_reference<coarse_grained_iterative> process(Container& a, int steps, const Update& update) {

				// return handle to asynchronous execution
				return async([&a,steps,update]{

					// iterative implementation
					Container b(a.size());

					Container* x = &a;
					Container* y = &b;

					using iter_type = decltype(a.size());

					for(int t=0; t<steps; t++) {

						// loop based parallel implementation with blocking synchronization
						pfor(iter_type(0),a.size(),[x,y,t,update](const iter_type& i){
							(*y)[i] = update(t,i,*x);
						});

						// swap buffers
						std::swap(x,y);
					}

					// make sure result is in a
					if (x != &a) {
						// move final data to the original container
						std::swap(a,b);
					}

				});

			}
		};


		struct fine_grained_iterative {

			template<typename Container, typename Update>
			stencil_reference<fine_grained_iterative> process(Container& a, int steps, const Update& update) {

				// return handle to asynchronous execution
				return async([&a,steps,update]{

					// iterative implementation
					Container b(a.size());

					Container* x = &a;
					Container* y = &b;

					using iter_type = decltype(a.size());

					detail::loop_reference<iter_type> ref;

					for(int t=0; t<steps; t++) {

						// loop based parallel implementation with fine grained dependencies
						ref = pfor(iter_type(0),a.size(),[x,y,t,update](const iter_type& i){
							(*y)[i] = update(t,i,*x);
						}, neighborhood_sync(ref));

						// swap buffers
						std::swap(x,y);
					}

					// make sure result is in a
					if (x != &a) {
						// move final data to the original container
						std::swap(a,b);
					}

					// wait for the task completion
					ref.wait();

				});

			}
		};



		// -- Recursive Stencil Implementation ---------------------------------------------------------

		namespace detail {

			using index_type = Coordinate<0>::element_type;
			using time_type = std::size_t;


			template<size_t dims>
			using Slopes = data::Vector<index_type,dims>;

			template<size_t dims>
			class Base {
			public:

				struct range {
					index_type begin;
					index_type end;
				};

				std::array<range,dims> boundaries;

				static Base zero() {
					return full(0);
				}

				static Base full(std::size_t size) {
					static_assert(dims == 1, "This constructor only supports 1-d bases.");
					Base res;
					res.boundaries[0] = { 0, (index_type)size };
					return res;
				}

				template<typename T>
				static Base full(const data::Vector<T,dims>& size) {
					Base res;
					for(size_t i=0; i<dims; i++) {
						res.boundaries[i] = { 0, size[i] };
					}
					return res;
				}

				bool empty() const {
					return size() == 0;
				}

				std::size_t size() const {
					std::size_t res = 1;
					for(const auto& cur : boundaries) {
						if (cur.begin >= cur.end) return 0;
						res *= (cur.end - cur.begin);
					}
					return res;
				}

				Coordinate<dims> extend() const {
					Coordinate<dims> res;
					for(size_t i = 0; i<dims; i++) {
						res[i] = boundaries[i].end - boundaries[i].begin;
					}
					return res;
				}

				index_type getMinimumWidth() const {
					index_type res = boundaries[0].end - boundaries[0].begin;
					for(size_t i = 1; i<dims; i++) {
						res = std::min(res,boundaries[i].end - boundaries[i].begin);
					}
					return res;
				}

				const range& operator[](size_t i) const {
					return boundaries[i];
				}

				range& operator[](size_t i) {
					return boundaries[i];
				}

				Base operator+(const Coordinate<dims>& other) const {
					Base res;
					for(size_t i=0; i<dims; i++) {
						res.boundaries[i] = { boundaries[i].begin + other[i], boundaries[i].end + other[i] };
					}
					return res;
				}

				friend std::ostream& operator<<(std::ostream& out, const Base& base) {
					if (dims == 0) return out << "[]";
					out << "[";
					for(size_t i=0; i<dims-1; i++) {
						out << base.boundaries[i].begin << "-" << base.boundaries[i].end << ",";
					}
					out  << base.boundaries[dims-1].begin << "-" << base.boundaries[dims-1].end;
					return out << "]";
				}
			};


			template<size_t dim>
			struct plain_scanner {

				plain_scanner<dim-1> nested;

				template<size_t full_dim, typename Lambda>
				void operator()(const Base<full_dim>& base, const Lambda& lambda, Coordinate<full_dim>& pos, int t) const {
					for(pos[dim]=base[dim].begin; pos[dim]<base[dim].end; pos[dim]++) {
						 nested(base, lambda, pos, t);
					}
				}

			};

			template<>
			struct plain_scanner<0> {

				template<size_t full_dim, typename Lambda>
				void operator()(const Base<full_dim>& base, const Lambda& lambda, Coordinate<full_dim>& pos, int t) const {
					for(pos[0]=base[0].begin; pos[0]<base[0].end; pos[0]++) {
						lambda(pos, t);
					}
				}

			};

			template<size_t dims>
			class Zoid {

				Base<dims> base;			// the projection of the zoid to the space dimensions

				Slopes<dims> slopes;		// the direction of the slopes

				time_type t_begin;			// the start time
				time_type t_end;			// the end time

			public:

	//			Zoid() : base(), slopes(1), t0(0), t1(0) {}

				Zoid(const Base<dims>& base, const Slopes<dims>& slopes, size_t t_begin, size_t t_end)
					: base(base), slopes(slopes), t_begin(t_begin), t_end(t_end) {}

	//			vector<vector<Zoid>> splitTime() const {
	//				auto split = getHeight() / 2;
	//
	//				Base<dims> baseA = base;
	//				Base<dims> baseB = base;
	//
	//				for(size_t i=0; i<dims; i++) {
	//					auto slope = slopes[i];
	//					if (slope < 0) {
	//						baseA[i].first = baseA[i].first + slope * split;
	//						baseA[i].second = baseA[i].second - slope * split;
	//					} else {
	//						baseB[i].first = baseB[i].first + slope * split;
	//						baseB[i].second = baseB[i].second - slope * split;
	//					}
	//				}
	//
	//				return vector<vector<Zoid>>({
	//					vector<Zoid>({ Zoid(baseA, slopes, t0, t0+split) }),
	//					vector<Zoid>({ Zoid(baseB, slopes, t0+split, t1) })
	//				});
	//			}
	//
	//			vector<vector<Zoid>> splitSpace() const {
	//
	//				// find longest dimension
	//				int max_dim;
	//				int max_width = 0;
	//				for(size_t i=0; i<dims; i++) {
	//					int width = getWidth(i);
	//					if (width>max_width) {
	//						max_width = width;
	//						max_dim = i;
	//					}
	//				}
	//
	//				// the max dimension is the split dimensin
	//				auto split_dim = max_dim;
	//
	//				// check whether longest dimension can be split
	//				assert(splitable(split_dim));
	//
	//				// create 3 fragments
	//				Zoid a = *this;
	//				Zoid b = *this;
	//				Zoid c = *this;
	//
	//				// get the split point
	//				auto center = (base.boundaries[split_dim].first + base.boundaries[split_dim].second) / 2;
	//				auto left = center;
	//				auto right = center;
	//
	//				if (slopes[split_dim] < 0) {
	//					auto hight = getHeight();
	//					left -= hight;
	//					right += hight;
	//				}
	//
	//				a.base.boundaries[split_dim].second = left;
	//				b.base.boundaries[split_dim] = { left, right };
	//				c.base.boundaries[split_dim].first = right;
	//
	//				// invert direction of center piece
	//				b.slopes[split_dim] *= -1;
	//
	//				// add fragments in right order
	//				if (slopes[split_dim] < 0) {
	//					return vector<vector<Zoid>>({
	//						vector<Zoid>({ b }),
	//						vector<Zoid>({ a,c })
	//					});
	//				}
	//
	//				// return positive order
	//				return vector<vector<Zoid>>({
	//					vector<Zoid>({ a, c }),
	//					vector<Zoid>({ b })
	//				});
	//			}
	//
	//			vector<vector<Zoid>> split() const {
	//				assert(!isTerminal() && "Do not split terminal Zoid!");
	//
	//				// try a space split
	//				if (spaceSplitable()) {
	//					return splitSpace();
	//				}
	//
	//				// fall back to time split
	//				return splitTime();
	//			}
	//
	//			bool isTerminal() const {
	//				return getFootprint() < 100;			// todo: find better limit
	//			}


				template<typename Lambda>
				void forEach(const Lambda& lambda) const {

					// create the plain scanner
					plain_scanner<dims-1> scanner;

					Coordinate<dims> x;
					auto plainBase = base;

					// over the time
					for(size_t t = t_begin; t < t_end; ++t) {

						// process this plain
						scanner(plainBase, lambda, x, t);

						// update the plain for the next level
						for(size_t i=0; i<dims; ++i) {
							plainBase[i].begin  += slopes[i];
							plainBase[i].end    -= slopes[i];
						}
					}

				}

	//			int getWidth(int dim) const {
	//				int res = base.width(dim);
	//				if (slopes[dim] < 0) res += 2*getHeight();
	//				return res;
	//			}
	//
	//			int getHeight() const {
	//				return t1-t0;
	//			}
	//
	//			bool spaceSplitable() const {
	//				for(size_t i=0; i<dims; i++) {
	//					if (splitable(i)) return true;
	//				}
	//				return false;
	//			}
	//
	//			bool splitable(int dim) const {
	//				return (slopes[dim] > 0) ?
	//					base.width(dim) > 4*getHeight()
	//				:
	//					base.width(dim) > 2*getHeight()
	//				;
	//			}
	//
	//			int getFootprint() const {
	//				int size = 1;
	//				for(size_t i=0; i<dims; i++) {
	//					size *= base.width(i);				// todo: this is wrong!
	//				}
	//				return size;
	//			}

				friend std::ostream& operator<<(std::ostream& out, const Zoid& zoid) {
					return out << "Zoid(" << zoid.base << "," << zoid.slopes << "," << zoid.t_begin << "-" << zoid.t_end << ")";
				}

			};


			template<unsigned Dims>
			struct container_info_base {
				constexpr static const unsigned dimensions = Dims;
				using base_type = Base<Dims>;
			};


			template<typename Container>
			struct container_info : public container_info_base<1> {
				using index_type = detail::index_type;
			};

			template<typename T, std::size_t Dims>
			struct container_info<data::Grid<T,Dims>> : public container_info_base<Dims> {
				using index_type = data::Vector<detail::index_type,Dims>;
			};


			template<typename Container>
			struct coordinate_converter {
				auto& operator()(const Coordinate<1>& pos) {
					return pos[0];
				}
			};

			template<typename T, std::size_t Dims>
			struct coordinate_converter<data::Grid<T,Dims>> {
				auto& operator()(const Coordinate<Dims>& pos) {
					return pos;
				}
			};

		}


		struct recursive_stencil {

			template<typename Container, typename Update>
			stencil_reference<recursive_stencil> process(Container& a, int steps, const Update& update) {

				using namespace detail;

				const unsigned dims = container_info<Container>::dimensions;
				using base_t = typename container_info<Container>::base_type;

				// iterative implementation
				Container b(a.size());

				Container* x = &a;
				Container* y = &b;

				// TODO: make parallel

				// get size of structure
				base_t base = base_t::full(a.size());
				auto size = base.extend();

				// the the smallest width (this is the limiting factor for the height)
				auto width = base.getMinimumWidth();

				// get the height of the largest zoids, thus the height of each layer
				auto height = width/2;

				// wrap update function into zoid-interface adapter
				auto wrappedUpdate = [&](const Coordinate<dims>& pos, time_t t){
					coordinate_converter<Container> conv;
					auto p = conv(data::elementwiseModulo(pos,size));
					if (t % 2) {
						(*y)[p] = update(t,p,*x);
					} else {
						(*x)[p] = update(t,p,*y);
					}
				};


				// compute base area partitioning
				struct split {
					typename base_t::range left;
					typename base_t::range right;
				};
				std::array<split,dims> splits;
				for(std::size_t i = 0; i<dims; i++) {
					auto curWidth = size[i];
					auto mid = curWidth - (curWidth - width) / 2;
					splits[i].left.begin  = 0;
					splits[i].left.end    = mid;
					splits[i].right.begin = mid;
					splits[i].right.end   = curWidth;
				}

				// process time layer by layer
				for(int t0=0; t0<steps; t0+=height) {

					// get the top of the current layer
					auto t1 = std::min<std::size_t>(t0+height,steps);

					// create the list of zoids to be processed
					std::vector<std::vector<Zoid<dims>>> zoids(dims+1);

					// generate binary patterns from 0 to 2^dims - 1
					for(size_t i=0; i < (1<<dims); i++) {

						// get base and slopes of the current zoid
						Base<dims> curBase = base;
						Slopes<dims> slopes;

						// move base to center on field, edge, or corner
						for(size_t j=0; j<dims; j++) {
							if (i & (1<<j)) {
								slopes[j] = -1;
								curBase.boundaries[j] = splits[j].right;
							} else {
								slopes[j] = 1;
								curBase.boundaries[j] = splits[j].left;
							}
						}

						// count the number of ones -- this determines the execution order
						int num_ones = 0;
						for(size_t j=0; j<dims; j++) {
							if (i & (1<<j)) num_ones++;
						}

						// add to execution plan
						zoids[num_ones].push_back(Zoid<dims>(curBase, slopes, t0, t1));
					}


					// process those zoids in order
					for(const auto& layer : zoids) {
						pfor(layer,[&](const Zoid<dims>& cur) {
							cur.forEach(wrappedUpdate);
						});
//						for(const auto& cur : layer) {
//							cur.forEach(wrappedUpdate);
//						}
					}

				}

				// make sure the result is in the a copy
				if (!(steps % 2)) {
					std::swap(a,b);
				}

				// done
				return {};
			}
		};

	} // end namespace implementation


} // end namespace user
} // end namespace api
} // end namespace allscale
