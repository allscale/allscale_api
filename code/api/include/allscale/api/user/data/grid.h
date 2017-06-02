#pragma once

#include <cstring>
#include <memory>

#include "allscale/api/core/data.h"

#include "allscale/api/user/operator/pfor.h"

#include "allscale/utils/assert.h"
#include "allscale/utils/printer/join.h"
#include "allscale/utils/large_array.h"
#include "allscale/utils/vector.h"

namespace allscale {
namespace api {
namespace user {
namespace data {


	// ---------------------------------------------------------------------------------
	//								 Declarations
	// ---------------------------------------------------------------------------------


	using coordinate_type = std::int64_t;

	template<std::size_t Dims>
	using GridPoint = utils::Vector<coordinate_type,Dims>;

	template<std::size_t Dims>
	class GridBox;

	template<std::size_t Dims>
	class GridRegion;

	template<typename T, std::size_t Dims>
	class GridFragment;

	template<typename T, std::size_t Dims = 2>
	class Grid;




	// ---------------------------------------------------------------------------------
	//								  Definitions
	// ---------------------------------------------------------------------------------

	namespace detail {

		template<std::size_t I>
		struct difference_computer {

			template<std::size_t Dims>
			void collectDifferences(const GridBox<Dims>& a, const GridBox<Dims>& b, GridBox<Dims>& cur, std::vector<GridBox<Dims>>& res) {
				std::size_t i = I-1;

				// if b is within a
				if (a.min[i] <= b.min[i] && b.max[i] <= a.max[i]) {

					// cover left part
					cur.min[i] = a.min[i]; cur.max[i] = b.min[i];
					if (cur.min[i] < cur.max[i]) difference_computer<I-1>().collectDifferences(a,b,cur,res);

					// cover center part
					cur.min[i] = b.min[i]; cur.max[i] = b.max[i];
					if (cur.min[i] < cur.max[i]) difference_computer<I-1>().collectDifferences(a,b,cur,res);

					// cover right part
					cur.min[i] = b.max[i]; cur.max[i] = a.max[i];
					if (cur.min[i] < cur.max[i]) difference_computer<I-1>().collectDifferences(a,b,cur,res);

				// if a is within b
				} else if (b.min[i] <= a.min[i] && a.max[i] <= b.max[i]) {

					// cover inner part
					cur.min[i] = a.min[i]; cur.max[i] = a.max[i];
					if (cur.min[i] < cur.max[i]) difference_computer<I-1>().collectDifferences(a,b,cur,res);

				// if a is on the left
				} else if (a.min[i] <= b.min[i]) {

					// cover left part
					cur.min[i] = a.min[i]; cur.max[i] = b.min[i];
					if (cur.min[i] < cur.max[i]) difference_computer<I-1>().collectDifferences(a,b,cur,res);

					// cover right part
					cur.min[i] = b.min[i]; cur.max[i] = a.max[i];
					if (cur.min[i] < cur.max[i]) difference_computer<I-1>().collectDifferences(a,b,cur,res);

				// otherwise a is on the right
				} else {

					// cover left part
					cur.min[i] = a.min[i]; cur.max[i] = b.max[i];
					if (cur.min[i] < cur.max[i]) difference_computer<I-1>().collectDifferences(a,b,cur,res);

					// cover right part
					cur.min[i] = b.max[i]; cur.max[i] = a.max[i];
					if (cur.min[i] < cur.max[i]) difference_computer<I-1>().collectDifferences(a,b,cur,res);

				}

			}

		};

		template<>
		struct difference_computer<0> {

			template<std::size_t Dims>
			void collectDifferences(const GridBox<Dims>&, const GridBox<Dims>& b, GridBox<Dims>& cur, std::vector<GridBox<Dims>>& res) {
				if(!b.covers(cur) && !cur.empty()) res.push_back(cur);
			}
		};

		template<std::size_t I>
		struct box_fuser {
			template<std::size_t Dims>
			bool apply(std::vector<GridBox<Dims>>& boxes) {

				// try fuse I-th dimension
				for(std::size_t i = 0; i<boxes.size(); i++) {
					for(std::size_t j = i+1; j<boxes.size(); j++) {

						// check whether a fusion is possible
						GridBox<Dims>& a = boxes[i];
						GridBox<Dims>& b = boxes[j];
						if (GridBox<Dims>::template areFusable<I-1>(a,b)) {

							// fuse the boxes
							GridBox<Dims> f = GridBox<Dims>::template fuse<I-1>(a,b);
							boxes.erase(boxes.begin() + j);
							boxes[i] = f;

							// start over again
							apply(boxes);
							return true;
						}
					}
				}

				// fuse smaller dimensions
				if (box_fuser<I-1>().apply(boxes)) {
					// start over again
					apply(boxes);
					return true;
				}

				// no more changes
				return false;
			}
		};

		template<>
		struct box_fuser<0> {
			template<std::size_t Dims>
			bool apply(std::vector<GridBox<Dims>>&) { return false; }
		};

		template<std::size_t I>
		struct line_scanner {
			template<std::size_t Dims, typename Lambda>
			void apply(const GridBox<Dims>& box, GridPoint<Dims>& a, GridPoint<Dims>& b, const Lambda& body) {
				for(coordinate_type i = box.min[Dims-I]; i < box.max[Dims-I]; ++i ) {
					a[Dims-I] = i;
					b[Dims-I] = i;
					line_scanner<I-1>().template apply<Dims>(box,a,b,body);
				}
			}
		};

		template<>
		struct line_scanner<1> {
			template<std::size_t Dims, typename Lambda>
			void apply(const GridBox<Dims>& box, GridPoint<Dims>& a, GridPoint<Dims>& b, const Lambda& body) {
				a[Dims-1] = box.min[Dims-1];
				b[Dims-1] = box.max[Dims-1];
				body(a,b);
			}
		};
	}


	template<std::size_t Dims>
	class GridBox {

		static_assert(Dims >= 1, "0-dimension Grids (=Skalars) not supported yet.");

		template<std::size_t I>
		friend struct detail::difference_computer;

		template<std::size_t I>
		friend struct detail::line_scanner;

		template<std::size_t D>
		friend class GridRegion;

		using point_type = GridPoint<Dims>;

		point_type min;
		point_type max;

		GridBox() {}

	public:

		GridBox(coordinate_type N)
			: min(0), max(N) {}

		GridBox(coordinate_type A, coordinate_type B)
			: min(A), max(B) {}

		GridBox(const point_type& N)
			: min(0), max(N) {}

		GridBox(const point_type& A, const point_type& B)
			: min(A), max(B) {}

		bool empty() const {
			return !min.strictlyDominatedBy(max);
		}

		std::size_t area() const {
			std::size_t res = 1;
			for(std::size_t i=0; i<Dims; i++) {
				if (max[i] <= min[i]) return 0;
				res *= max[i] - min[i];
			}
			return res;
		}

		bool operator==(const GridBox& other) const {
			if (this == &other) return true;
			if (empty() && other.empty()) return true;
			return min == other.min && max == other.max;
		}

		bool operator!=(const GridBox& other) const {
			return !(*this == other);
		}

		bool covers(const point_type& point) const {
			for(std::size_t i = 0; i<Dims; i++) {
				if (!(min[i] <= point[i] && point[i] < max[i])) return false;
			}
			return true;
		}

		bool covers(const GridBox& other) const {
			if (other.empty()) return true;
			if (empty()) return false;
			return min.dominatedBy(other.min) && other.max.dominatedBy(max);
		}

		bool intersectsWith(const GridBox& other) const {
			// empty sets do not intersect with any set
			if (other.empty() || empty()) return false;

			// check each dimension
			for(std::size_t i = 0; i<Dims; i++) {
				// the minimum of the one has to be between the min and max of the other
				if (!(
						(min[i] <= other.min[i] && other.min[i] < max[i]) ||
						(other.min[i] <= min[i] && min[i] < other.max[i])
					)) {
					return false;		// if not, there is no intersection
				}
			}
			return true;
		}

		static std::vector<GridBox> merge(const GridBox& a, const GridBox& b) {

			// handle empty sets
			if (a.empty() && b.empty()) return std::vector<GridBox>();
			if (a.empty()) return std::vector<GridBox>({b});
			if (b.empty()) return std::vector<GridBox>({a});

			// boxes are intersecting => we have to do some work
			auto res = difference(a,b);
			res.push_back(b);
			return res;
		}

		static GridBox intersect(const GridBox& a, const GridBox& b) {
			// compute the intersection
			GridBox res = a;
			for(std::size_t i = 0; i<Dims; i++) {
				res.min[i] = std::max(res.min[i],b.min[i]);
				res.max[i] = std::min(res.max[i],b.max[i]);
			}
			return res;
		}

		static std::vector<GridBox> difference(const GridBox& a, const GridBox& b) {

			// handle case where b covers whole a
			if (b.covers(a)) return std::vector<GridBox>();

			// check whether there is an actual intersection
			if (!a.intersectsWith(b)) {
				return std::vector<GridBox>({a});
			}

			// slice up every single dimension
			GridBox cur;
			std::vector<GridBox> res;
			detail::difference_computer<Dims>().collectDifferences(a,b,cur,res);
			return res;
		}

		template<typename Lambda>
		void scanByLines(const Lambda& body) const {
			if (empty()) return;
			point_type a;
			point_type b;
			detail::line_scanner<Dims>().template apply<Dims>(*this,a,b,body);
		}

		template<std::size_t D>
		static bool areFusable(const GridBox& a, const GridBox& b) {
			static_assert(D < Dims, "Can not fuse on non-existing dimension.");
			if (a.min > b.min) return areFusable<D>(b,a);
			if (a.max[D] != b.min[D]) return false;
			for(std::size_t i = 0; i<Dims; i++) {
				if (i == D) continue;
				if (a.min[i] != b.min[i]) return false;
				if (a.max[i] != b.max[i]) return false;
			}
			return true;
		}

		template<std::size_t D>
		static GridBox fuse(const GridBox& a, const GridBox& b) {
			assert_true(areFusable<D>(a,b));
			if (a.min[D] > b.min[D]) return fuse<D>(b,a);
			GridBox res = a;
			res.max[D] = b.max[D];
			return res;
		}

		friend std::ostream& operator<<(std::ostream& out, const GridBox& box) {
			return out << "[" << box.min << " - " << box.max << "]";
		}

	};

	// TODO: implement grid-boxes of 0-dimension (scalars)
	template<>
	class GridBox<0> {

	};

	template<std::size_t Dims>
	class GridRegion {

		using point_type = GridPoint<Dims>;
		using box_type = GridBox<Dims>;

		point_type total;
		std::vector<box_type> regions;

	public:

		GridRegion(const point_type& total = 0) : total(total) {}

		GridRegion(const point_type& total, coordinate_type N)
			: total(total), regions({box_type(N)}) {
			if (0 >= N) regions.clear();
			else assert_true(regions[0].max.dominatedBy(total));
		}

		GridRegion(const point_type& total, coordinate_type A, coordinate_type B)
			: total(total), regions({box_type(A,B)}) {
			if (A >= B) regions.clear();
			else assert_true(regions[0].max.dominatedBy(total));
		}

		GridRegion(const point_type& total, const point_type& size)
			: total(total), regions({box_type(0,size)}) {
			if (regions[0].empty()) regions.clear();
			else assert_true(regions[0].max.dominatedBy(total));
		}

		GridRegion(const point_type& total, const point_type& min, const point_type& max)
			: total(total), regions({box_type(min,max)}) {
			assert_true(min.dominatedBy(total));
			assert_true(max.dominatedBy(total));
			assert_true(min.dominatedBy(max));
		}

		GridRegion(const GridRegion&) = default;
		GridRegion(GridRegion&&) = default;

		GridRegion& operator=(const GridRegion&) = default;
		GridRegion& operator=(GridRegion&&) = default;

		const point_type& getTotal() const {
			return total;
		}

		box_type boundingBox() const {
			// handle empty region
			if (regions.empty()) return box_type(0);

			// if there is a single element
			if (regions.size() == 1u) return regions.front();

			// compute the bounding box
			box_type res = regions.front();
			for(const box_type& cur : regions) {
				res.min = utils::elementwiseMin(res.min, cur.min);
				res.max = utils::elementwiseMax(res.max, cur.max);
			}
			return res;
		}

		bool operator==(const GridRegion& other) const {
			return difference(*this,other).empty() && other.difference(other,*this).empty();
		}

		bool operator!=(const GridRegion& other) const {
			return regions != other.regions;
		}

		bool empty() const {
			return regions.empty();
		}

		static GridRegion merge(const GridRegion& a, const GridRegion& b) {

			// if both sets are empty => done
			if(a.empty() && b.empty()) return a;

			// build result
			GridRegion res = a;

			// combine total
			if (res.total == 0) res.total = b.total;
			assert_true(a.total == 0 || b.total == 0 || a.total == b.total);
			assert_false(res.total == 0);

			// combine regions
			for(const auto& cur : difference(b,a).regions) {
				res.regions.push_back(cur);
			}

			// compress result
			res.compress();

			// done
			return res;
		}

		template<typename ... Rest>
		static GridRegion merge(const GridRegion& a, const GridRegion& b, const Rest& ... rest) {
			return merge(merge(a,b),rest...);
		}

		static GridRegion intersect(const GridRegion& a, const GridRegion& b) {

			// if one of the sets is empty => done
			if(a.empty()) return a;
			if(b.empty()) return b;

			// build result
			GridRegion res;

			// combine total
			if (res.total == 0) res.total = b.total;
			assert_true(a.total == 0 || b.total == 0 || a.total == b.total);
			assert_false(res.total == 0);

			// combine regions
			for(const auto& curA : a.regions) {
				for(const auto& curB : b.regions) {
					box_type diff = box_type::intersect(curA,curB);
					if (!diff.empty()) {
						res.regions.push_back(diff);
					}
				}
			}

			// compress result
			res.compress();

			// done
			return res;
		}

		static GridRegion difference(const GridRegion& a, const GridRegion& b) {

			// handle empty sets
			if(a.empty() || b.empty()) return a;


			// build result
			GridRegion res = a;

			// combine total
			if (res.total == 0) res.total = b.total;
			assert_true(a.total == 0 || b.total == 0 || a.total == b.total);
			assert_false(res.total == 0);

			// combine regions
			for(const auto& curB : b.regions) {
				std::vector<box_type> next;
				for(const auto& curA : res.regions) {
					for(const auto& n : box_type::difference(curA,curB)) {
						next.push_back(n);
					}
				}
				res.regions.swap(next);
			}

			// compress result
			res.compress();

			// done
			return res;
		}

		/**
		 * Scans the covered range, line by line.
		 */
		template<typename Lambda>
		void scanByLines(const Lambda& body) const {
			for(const auto& cur : regions) {
				cur.scanByLines(body);
			}
		}

		/**
		 * An operator to load an instance of this range from the given archive.
		 */
		static GridRegion load(utils::Archive&) {
			assert_not_implemented();
			return GridRegion();
		}

		/**
		 * An operator to store an instance of this range into the given archive.
		 */
		void store(utils::Archive&) const {
			assert_not_implemented();
			// nothing so far
		}

		friend std::ostream& operator<<(std::ostream& out, const GridRegion& region) {
			return out << "{" << utils::join(",",region.regions) << "}";
		}

	private:

		void compress() {
			// try to fuse boxes
			detail::box_fuser<Dims>().apply(regions);
		}

	};


	template<typename T, std::size_t Dims>
	class GridFragment {
	public:

		using shared_data_type = core::no_shared_data;
		using facade_type = Grid<T,Dims>;
		using region_type = GridRegion<Dims>;

	private:

		using point = GridPoint<Dims>;
		using box = GridBox<Dims>;

		region_type size;

		utils::LargeArray<T> data;

	public:

		GridFragment(const region_type& size)
			: GridFragment(core::no_shared_data(), size) {}

		GridFragment(const core::no_shared_data&, const region_type& size) : size(size), data(area(size.getTotal())) {
			// allocate covered data space
			size.scanByLines([&](const point& a, const point& b) {
				data.allocate(flatten(a),flatten(b));
			});
		}

		T& operator[](const point& pos) {
			return data[flatten(pos)];
		}

		const T& operator[](const point& pos) const {
			return data[flatten(pos)];
		}

		Grid<T,Dims> mask() {
			return Grid<T,Dims>(*this);
		}

		const region_type& getCoveredRegion() const {
			return size;
		}

		const point& totalSize() const {
			return size.getTotal();
		}

		void resize(const region_type& newSize) {

			// if the fragment is empty so far
			if (size.getTotal() == 0) {
				// initialize the large array
				size = region_type(newSize.getTotal());
				data = utils::LargeArray<T>(area(size.getTotal()));
			}

			// check that size does not change
			assert_eq(size.getTotal(),newSize.getTotal());

			// get the difference
			region_type plus  = region_type::difference(newSize,size);
			region_type minus = region_type::difference(size,newSize);

			// update the size
			size = newSize;

			// allocated new data
			plus.scanByLines([&](const point& a, const point& b){
				data.allocate(flatten(a),flatten(b));
			});

			// free excessive memory
			minus.scanByLines([&](const point& a, const point& b){
				data.free(flatten(a),flatten(b));
			});
		}

		void insert(const GridFragment& other, const region_type& area) {
			assert_true(core::isSubRegion(area,other.size)) << "New data " << area << " not covered by source of size " << size << "\n";
			assert_true(core::isSubRegion(area,size))       << "New data " << area << " not covered by target of size " << size << "\n";

			// copy data line by line using memcpy
			area.scanByLines([&](const point& a, const point& b){
				auto start = flatten(a);
				auto length = (flatten(b) - start) * sizeof(T);
				std::memcpy(&data[start],&other.data[start],length);
			});
		}

		void save(utils::Archive& /*a*/, const region_type& /*keys*/) const {
			assert_not_implemented();
		}

		void load(utils::Archive& /*a*/) {
			assert_not_implemented();
		}

	private:

		static std::size_t area(const GridPoint<Dims>& pos) {
			std::size_t res = 1;
			for(std::size_t i=0; i<Dims; ++i) {
				res *= pos[i];
			}
			return res;
		}

		coordinate_type flatten(const GridPoint<Dims>& pos) const {
			coordinate_type res = 0;
			coordinate_type size = 1;

			for(int i=Dims-1; i>=0; i--) {
				res += pos[i] * size;
				size *= this->size.getTotal()[i];
			}

			return res;
		}

	};

	template<typename T, std::size_t Dims>
	class Grid : public core::data_item<GridFragment<T,Dims>> {

		/**
		 * A pointer to an underlying fragment owned if used in an unmanaged state.
		 */
		std::unique_ptr<GridFragment<T,Dims>> owned;

		/**
		 * A reference to the fragment instance operating on, referencing the owned fragment or an externally managed one.
		 */
		GridFragment<T,Dims>* base;

		/**
		 * Enables fragments to use the private constructor below.
		 */
		friend class GridFragment<T,Dims>;

		/**
		 * The constructor to be utilized by the fragment to create a facade for an existing fragment.
		 */
		Grid(GridFragment<T,Dims>& base) : base(&base) {}

	public:

		/**
		 * The type of coordinate utilized by this type.
		 */
		using coordinate_type = GridPoint<Dims>;

		/**
		 * The type of region utilized by this type.
		 */
		using region_type = GridRegion<Dims>;

		/**
		 * Creates a new map covering the given region.
		 */
		Grid(const coordinate_type& size)
			: owned(std::make_unique<GridFragment<T,Dims>>(region_type(size,0,size))), base(owned.get()) {}

		/**
		 * Disable copy construction.
		 */
		Grid(const Grid&) = delete;

		/**
		 * Enable move construction.
		 */
		Grid(Grid&&) = default;

		/**
		 * Disable copy-assignments.
		 */
		Grid& operator=(const Grid&) = delete;

		/**
		 * Enable move assignments.
		 */
		Grid& operator=(Grid&&) = default;

		/**
		 * Obtains the full size of this grid.
		 */
		coordinate_type size() const {
			return base->totalSize();
		}

		/**
		 * Provides read/write access to one of the values stored within this grid.
		 */
		T& operator[](const coordinate_type& index) {
			return (*base)[index];
		}

		/**
		 * Provides read access to one of the values stored within this grid.
		 */
		const T& operator[](const coordinate_type& index) const {
			return (*base)[index];
		}

		/**
		 * A sequential scan over all elements within this grid, providing
		 * read-only access.
		 */
		template<typename Op>
		void forEach(const Op& op) const {
			allscale::api::user::detail::forEach(
					coordinate_type(0),
					size(),
					[&](const auto& pos){
						op((*this)[pos]);
					}
			);
		}

		/**
		 * A sequential scan over all elements within this grid, providing
		 * read/write access.
		 */
		template<typename Op>
		void forEach(const Op& op) {
			allscale::api::user::detail::forEach(
					coordinate_type(0),
					size(),
					[&](const auto& pos){
						op((*this)[pos]);
					}
			);
		}

		/**
		 * A parallel scan over all elements within this grid, providing
		 * read-only access.
		 */
		template<typename Op>
		auto pforEach(const Op& op) const {
			return pfor(coordinate_type(0), size(), [&](const auto& pos) { op((*this)[pos]); });
		}

		/**
		 * A parallel scan over all elements within this grid, providing
		 * read/write access.
		 */
		template<typename Op>
		auto pforEach(const Op& op) {
			return pfor(coordinate_type(0), size(), [&](const auto& pos) { op((*this)[pos]); });
		}

	};

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
