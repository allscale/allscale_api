#pragma once

#include <array>

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
						pfor(iter_type(0),a.size(),[x,&a,update](const iter_type& i){
							a[i] = (*x)[i];
						});

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
						ref = pfor(iter_type(0),a.size(),[x,&a,update](const iter_type& i){
							a[i] = (*x)[i];
						}, neighborhood_sync(ref));

					}

					// wait for the task completion
					ref.wait();

				});

			}
		};



		// -- Recursive Stencil Implementation ---------------------------------------------------------

		namespace detail {

			using index_type = int64_t;
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

			private:

				std::array<range,dims> boundaries;

			public:

				static Base zero() {
					return full(0);
				}

				static Base full(const data::Vector<index_type,dims>& size) {
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


	//		template<size_t dims>
	//		Slopes<dims> ones() {
	//			Slopes<dims> res;
	//			for(size_t i=0; i<dims; i++) res[i] = 1;
	//			return res;
	//		}

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

			template<typename T, unsigned Dims>
			struct container_info<data::Grid<T,Dims>> : public container_info_base<Dims> {
				using index_type = data::Vector<detail::index_type,Dims>;
			};

		}


		struct recursive_stencil {

			template<typename Container, typename Update>
			stencil_reference<recursive_stencil> process(Container& a, int steps, const Update& update) {

				using namespace detail;

				using base_t = typename container_info<Container>::base_type;

				// iterative implementation
	//			Container b(a.size());
	//
	//			Container* x = &a;
	//			Container* y = &b;

				// TODO: make parallel

				// TODO: generalize data structure

				// get size of structure
				base_t base = base_t::full(a.size());

	//			detail::index_type size = a.size();
	//
	//
	//			// get step size (height of pyramid)
	//			auto step = std::min<std::size_t>(steps,size/2);
	//
	//			// wrap update function into zoid-interface adapter
	//			auto wrappedUpdate = [&](const Coordinate<1>& pos, time_t t){
	//				auto p = pos[0] % size;
	//				if (t % 2) {
	//					(*y)[p] = update(t,p,*x);
	//				} else {
	//					(*x)[p] = update(t,p,*y);
	//				}
	//			};
	//
	//			// decompose full space/time block in layers of height step
	//			for(int i=0; i<steps; i+=step) {
	//
	//				auto top = std::min<std::size_t>(i+step,steps);
	//
	//				// compute up-zoid
	//				auto baseUp = detail::Base<1>::full(data::Vector<detail::index_type,1>{size});
	//				detail::Zoid<1> up(baseUp,1,i,top);
	//				up.forEach(wrappedUpdate);
	//
	//				// compute down-zoid
	//				auto baseDown = detail::Base<1>::zero() + Coordinate<1>(size);
	//				detail::Zoid<1> down(baseDown,-1,i,top);
	//				down.forEach(wrappedUpdate);
	//
	//			}
	//
	//			// make sure the result is in a
	//			if (!(steps % 2)) {
	//				for(int i = 0; i<size; ++i) {
	//					a[i] = b[i];
	//				}
	//			}

				// done
				return {};
			}
		};

	} // end namespace implementation


} // end namespace user
} // end namespace api
} // end namespace allscale
