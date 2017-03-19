#pragma once

#include <array>
#include <utility>
#include <tuple>
#include <vector>

#include "allscale/api/user/data/grid.h"
#include "allscale/api/user/data/vector.h"

#include "allscale/api/user/operator/pfor.h"
#include "allscale/api/user/operator/async.h"
#include "allscale/api/user/operator/internal/operation_reference.h"

#include "allscale/utils/bitmanipulation.h"

namespace allscale {
namespace api {
namespace user {


	// ---------------------------------------------------------------------------------------------
	//									    Declarations
	// ---------------------------------------------------------------------------------------------


	template<std::size_t dims>
	using Coordinate = data::Vector<std::int64_t,dims>;

	namespace implementation {

		struct coarse_grained_iterative;

		struct fine_grained_iterative;

		struct sequential_recursive;

		struct parallel_recursive;

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

					user::detail::loop_reference<iter_type> ref;

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
						res[i] = getWidth(i);
					}
					return res;
				}

				index_type getWidth(std::size_t dim) const {
					return boundaries[dim].end - boundaries[dim].begin;
				}

				index_type getMinimumWidth() const {
					index_type res = getWidth(0);
					for(size_t i = 1; i<dims; i++) {
						res = std::min(res,getWidth(i));
					}
					return res;
				}

				index_type getMaximumWidth() const {
					index_type res = getWidth(0);
					for(size_t i = 1; i<dims; i++) {
						res = std::max(res,getWidth(i));
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

				Zoid() {}

				Zoid(const Base<dims>& base, const Slopes<dims>& slopes, size_t t_begin, size_t t_end)
					: base(base), slopes(slopes), t_begin(t_begin), t_end(t_end) {}


				template<typename Lambda>
				void forEach(const Lambda& lambda) const {

					// TODO: make this one cache oblivious

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


				template<typename Lambda>
				core::treeture<void> pforEach(const Lambda& lambda) const {

					// recursively decompose the covered space-time volume
					return core::prec(
						[](const Zoid& zoid) {
							// check whether this zoid can no longer be divided
							return zoid.isTerminal();
						},
						[&](const Zoid& zoid) {
							// process final steps sequentially
							zoid.forEach(lambda);
						},
						core::pick(
							[](const Zoid& zoid, const auto& rec) {
								// make sure the zoid is not terminal
								assert_false(zoid.isTerminal());

								// check whether it can be split in space
								if (!zoid.isSpaceSplitable()) {
									// we need a time split
									auto parts = zoid.splitTime();
									return core::sequential(
										rec(parts.bottom),
										rec(parts.top)
									);
								}

								// let's do a space split
								auto parts = zoid.splitSpace();

								// schedule depending on the orientation
								return (parts.opening)
										? core::sequential(
											rec(parts.c),
											core::parallel(
												rec(parts.l),
												rec(parts.r)
											)
										)
										: core::sequential(
											core::parallel(
												rec(parts.l),
												rec(parts.r)
											),
											rec(parts.c)
										);


							},
							[&](const Zoid& zoid, const auto&) {
								// provide sequential alternative
								zoid.forEach(lambda);
							}
						)
					)(*this);

				}


				/**
				 * The height of this zoid in temporal direction.
				 */
				int getHeight() const {
					return (int)t_end-t_begin;
				}

				/**
				 * Compute the number of elements this volume is covering
				 * when being projected to the space domain.
				 */
				int getFootprint() const {
					int size = 1;
					int dt = getHeight();
					for(size_t i=0; i<dims; i++) {
						auto curWidth = base.width(i);
						if (slopes[i] < 0) {
							curWidth += 2*dt;
						}
						size *= curWidth;
					}
					return size;
				}

				friend std::ostream& operator<<(std::ostream& out, const Zoid& zoid) {
					return out << "Zoid(" << zoid.base << "," << zoid.slopes << "," << zoid.t_begin << "-" << zoid.t_end << ")";
				}

			private:

				/**
				 * Tests whether this zoid can not be further divided.
				 */
				bool isTerminal() const {
					// reached when the height is 1 and width is 3
					return getHeight() <= 1 && base.getMaximumWidth() < 3;
				}


				/**
				 * Computes the width of the shadow projected of this zoid on
				 * the given space dimension.
				 */
				size_t getWidth(size_t dim) const {
					size_t res = base.getWidth(dim);
					if (slopes[dim] < 0) res += 2*getHeight();
					return res;
				}


				/**
				 * Determines whether this zoid is splitable in space.
				 */
				bool isSpaceSplitable() const {
					for(size_t i=0; i<dims; i++) {
						if (isSplitable(i)) return true;
					}
					return false;
				}

				/**
				 * Tests whether it can be split along the given space dimension.
				 */
				bool isSplitable(size_t dim) const {
					return getWidth(dim) > 4*getHeight();
				}

				// the result of a time split
				struct TimeDecomposition {
					Zoid top;
					Zoid bottom;
				};

				/**
				 * Splits this zoid in two sub-zoids along the time dimension. The
				 * First component will be the bottom, the second the top.
				 */
				TimeDecomposition splitTime() const {
					auto split = getHeight() / 2;

					Base<dims> mid = base;

					for(size_t i=0; i<dims; i++) {
						auto diff  = slopes[i] * split;
						mid[i].begin += diff;
						mid[i].end   -= diff;
					}

					return {
						Zoid(mid,   slopes, t_begin+split, t_end),		// top
						Zoid(base,  slopes, t_begin, t_begin+split)		// bottom
					};
				}


				// the result of a space split
				struct SpaceDecomposition {
					Zoid l;			// < the left fragment
					Zoid c;			// < the middle fragment
					Zoid r;			// < the right fragment
					bool opening;	// < determines whether the splitting dimension was opening (negative slope) or closing (positive slope)
				};

				/**
				 * Splits this zoid into three sub-zoids along the space dimension.
				 */
				SpaceDecomposition splitSpace() const {
					assert_true(isSpaceSplitable());

					// find longest dimension
					int max_dim;
					int max_width = 0;
					for(size_t i=0; i<dims; i++) {
						int width = getWidth(i);
						if (width>max_width) {
							max_width = width;
							max_dim = i;
						}
					}

					// the max dimension is the split dimensin
					auto split_dim = max_dim;

					// check whether longest dimension can be split
					assert(isSplitable(split_dim));

					// create 3 fragments
					SpaceDecomposition res {
						*this, *this, *this, (slopes[split_dim] < 0)
					};

					// get the split point
					auto center = (base.boundaries[split_dim].begin + base.boundaries[split_dim].end) / 2;
					auto left = center;
					auto right = center;

					if (slopes[split_dim] < 0) {
						auto hight = getHeight();
						left -= hight;
						right += hight;
					}

					res.l.base.boundaries[split_dim].end = left;
					res.c.base.boundaries[split_dim] = { left, right };
					res.r.base.boundaries[split_dim].begin = right;

					// invert direction of center piece
					res.c.slopes[split_dim] *= -1;

					// return decomposition
					return res;
				}

			};

			template<std::size_t Dims>
			class ExecutionPlan {

				using zoid_type = Zoid<Dims>;

				// the execution plan of one layer -- represented as an embedded hyper-cube
				using layer_plan = std::array<zoid_type,1 << Dims>;

				// the list of execution plans of all layers
				std::vector<layer_plan> layers;

			public:

				template<typename Op>
				void runSequential(const Op& op) const {
					const std::size_t num_tasks = 1 << Dims;

					// fill a vector with the indices of the tasks
					std::array<std::size_t,num_tasks> order;
					for(std::size_t i = 0; i<num_tasks;++i) {
						order[i] = i;
					}

					// sort the vector by the number of 1-bits
					std::sort(order.begin(),order.end(),[](std::size_t a, std::size_t b){
						return getNumBitsSet(a) < getNumBitsSet(b);
					});

					// process zoids in obtained order
					for(const auto& cur : layers) {
						for(const auto& idx : order) {
							cur[idx].forEach(op);
						}
					}
				}

				template<typename Op>
				core::treeture<void> runParallel(const Op& op) const {

					const std::size_t num_tasks = 1 << Dims;

					// fill a vector with the indices of the tasks
					std::array<std::size_t,num_tasks> order;
					for(std::size_t i = 0; i<num_tasks;++i) {
						order[i] = i;
					}

					// sort the vector by the number of 1-bits
					std::sort(order.begin(),order.end(),[](std::size_t a, std::size_t b){
						return getNumBitsSet(a) < getNumBitsSet(b);
					});

					// process zoids in obtained order
					// TODO: process independent zoids in parallel
					for(const auto& cur : layers) {
						for(const auto& idx : order) {
							cur[idx].pforEach(op).wait();
						}
					}

					// TODO: provide an asynchronous handle
					return core::done();
				}

				static ExecutionPlan create(const Base<Dims>& base, int steps) {

					// get size of structure
					auto size = base.extend();

					// the the smallest width (this is the limiting factor for the height)
					auto width = base.getMinimumWidth();

					// get the height of the largest zoids, thus the height of each layer
					auto height = width/2;

					// compute base area partitioning
					struct split {
						typename Base<Dims>::range left;
						typename Base<Dims>::range right;
					};
					std::array<split,Dims> splits;
					for(std::size_t i = 0; i<Dims; i++) {
						auto curWidth = size[i];
						auto mid = curWidth - (curWidth - width) / 2;
						splits[i].left.begin  = 0;
						splits[i].left.end    = mid;
						splits[i].right.begin = mid;
						splits[i].right.end   = curWidth;
					}

					// create the list of per-layer plans to be processed
					ExecutionPlan plan;

					// process time layer by layer
					for(int t0=0; t0<steps; t0+=height) {

						// get the top of the current layer
						auto t1 = std::min<std::size_t>(t0+height,steps);

						// create the list of zoids in this step
						plan.layers.emplace_back();
						layer_plan& zoids = plan.layers.back();

						// generate binary patterns from 0 to 2^dims - 1
						for(size_t i=0; i < (1<<Dims); i++) {

							// get base and slopes of the current zoid
							Base<Dims> curBase = base;
							Slopes<Dims> slopes;

							// move base to center on field, edge, or corner
							for(size_t j=0; j<Dims; j++) {
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
							for(size_t j=0; j<Dims; j++) {
								if (i & (1<<j)) num_ones++;
							}

							// add to execution plan
							zoids[i] = Zoid<Dims>(curBase, slopes, t0, t1);
						}

					}

					// build the final result
					return plan;
				}

			private:

				static std::size_t getNumBitsSet(std::size_t mask) {
					return utils::countOnes(mask);
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

		struct sequential_recursive {

			template<typename Container, typename Update>
			stencil_reference<sequential_recursive> process(Container& a, int steps, const Update& update) {

				using namespace detail;

				const unsigned dims = container_info<Container>::dimensions;
				using base_t = typename container_info<Container>::base_type;

				// iterative implementation
				Container b(a.size());

				// TODO:
				//  - switch internally to cache-oblivious access pattern (optional)

				// get size of structure
				base_t base = base_t::full(a.size());
				auto size = base.extend();

				// wrap update function into zoid-interface adapter
				auto wrappedUpdate = [&](const Coordinate<dims>& pos, time_t t){
					coordinate_converter<Container> conv;
					auto p = conv(data::elementwiseModulo(pos,size));
					if (t % 2) {
						b[p] = update(t,p,a);
					} else {
						a[p] = update(t,p,b);
					}
				};

				// get the execution plan
				auto exec_plan = ExecutionPlan<dims>::create(base,steps);

				// process the execution plan
				exec_plan.runSequential(wrappedUpdate);

				// make sure the result is in the a copy
				if (!(steps % 2)) {
					std::swap(a,b);
				}

				// done
				return {};
			}
		};


		struct parallel_recursive {

			template<typename Container, typename Update>
			stencil_reference<parallel_recursive> process(Container& a, int steps, const Update& update) {

				using namespace detail;

				const unsigned dims = container_info<Container>::dimensions;
				using base_t = typename container_info<Container>::base_type;

				// iterative implementation
				Container b(a.size());

				// TODO:
				//  - switch internally to cache-oblivious access pattern (optional)
				//  - make parallel with fine-grained dependencies

				// get size of structure
				base_t base = base_t::full(a.size());
				auto size = base.extend();

				// wrap update function into zoid-interface adapter
				auto wrappedUpdate = [&](const Coordinate<dims>& pos, time_t t){
					coordinate_converter<Container> conv;
					auto p = conv(data::elementwiseModulo(pos,size));
					if (t % 2) {
						b[p] = update(t,p,a);
					} else {
						a[p] = update(t,p,b);
					}
				};


				// get the execution plan
				auto exec_plan = ExecutionPlan<dims>::create(base,steps);

				// process the execution plan
				exec_plan.runParallel(wrappedUpdate);


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
