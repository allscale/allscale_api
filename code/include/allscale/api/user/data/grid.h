#pragma once

#include "allscale/api/core/data.h"
#include "allscale/api/user/data/vector.h"

#include "allscale/utils/assert.h"
#include "allscale/utils/printer/join.h"

namespace allscale {
namespace api {
namespace user {
namespace data {


	// ---------------------------------------------------------------------------------
	//								 Declarations
	// ---------------------------------------------------------------------------------


	using coordinate_type = std::size_t;

	template<unsigned Dims>
	using GridPoint = Vector<coordinate_type,Dims>;

	template<unsigned Dims>
	class GridBox;

	template<unsigned Dims>
	class GridRegion;

	template<typename T, unsigned Dims>
	class GridFragment;

	template<typename T, unsigned Dims = 2>
	class Grid;




	// ---------------------------------------------------------------------------------
	//								  Definitions
	// ---------------------------------------------------------------------------------

	namespace detail {

		template<unsigned I>
		struct difference_computer {

			template<unsigned Dims>
			void collectDifferences(const GridBox<Dims>& a, const GridBox<Dims>& b, GridBox<Dims>& cur, std::vector<GridBox<Dims>>& res) {
				unsigned i = I-1;

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

			template<unsigned Dims>
			void collectDifferences(const GridBox<Dims>&, const GridBox<Dims>& b, GridBox<Dims>& cur, std::vector<GridBox<Dims>>& res) {
				if(!b.covers(cur) && !cur.empty()) res.push_back(cur);
			}
		};

	}


	template<unsigned Dims>
	class GridBox {

		template<unsigned I>
		friend struct detail::difference_computer;

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

		bool operator==(const GridBox& other) const {
			if (this == &other) return true;
			if (empty() && other.empty()) return true;
			return min == other.min && max == other.max;
		}

		bool operator!=(const GridBox& other) const {
			return !(*this == other);
		}

		bool covers(const point_type& point) const {
			for(unsigned i = 0; i<Dims; i++) {
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
			for(unsigned i = 0; i<Dims; i++) {
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
			for(unsigned i = 0; i<Dims; i++) {
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

		friend std::ostream& operator<<(std::ostream& out, const GridBox& box) {
			return out << "[" << box.min << " - " << box.max << "]";
		}

	};

	// TODO: implement grid-boxes of 0-dimension (scalars)
	template<>
	class GridBox<0>;

	template<unsigned Dims>
	class GridRegion {

		using point_type = GridPoint<Dims>;
		using box_type = GridBox<Dims>;

		std::vector<box_type> regions;

	public:

		GridRegion() {}

		GridRegion(coordinate_type N)
			: regions({box_type(N)}) {
			if (0 >= N) regions.clear();
		}

		GridRegion(coordinate_type A, coordinate_type B)
			: regions({box_type(A,B)}) {
			if (A >= B) regions.clear();
		}

		GridRegion(const point_type& size)
			: regions({box_type(0,size)}) {
			if (regions[0].empty()) regions.clear();
		}

		GridRegion(const point_type& min, const point_type& max)
			: regions({box_type(min,max)}) {
			assert_true(min.dominatedBy(max));
		}

		GridRegion(const GridRegion&) = default;
		GridRegion(GridRegion&&) = default;

		GridRegion& operator=(const GridRegion&) = default;
		GridRegion& operator=(GridRegion&&) = default;

		box_type boundingBox() const {
			// handle empty region
			if (regions.empty()) return box_type(0);

			// compute the bounding box
			box_type res = regions.front();
			for(const box_type& cur : regions) {
				res.min = pointwiseMin(res.min, cur.min);
				res.max = pointwiseMax(res.max, cur.max);
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
			GridRegion res = a;
			for(const auto& cur : difference(b,a).regions) {
				res.regions.push_back(cur);
			}
			return res;
		}

		static GridRegion intersect(const GridRegion& a, const GridRegion& b) {
			GridRegion res;
			for(const auto& curA : a.regions) {
				for(const auto& curB : b.regions) {
					box_type diff = box_type::intersect(curA,curB);
					if (!diff.empty()) {
						res.regions.push_back(diff);
					}
				}
			}
			return res;
		}

		static GridRegion difference(const GridRegion& a, const GridRegion& b) {
			GridRegion res = a;
			for(const auto& curB : b.regions) {
				std::vector<box_type> next;
				for(const auto& curA : res.regions) {
					for(const auto& n : box_type::difference(curA,curB)) {
						next.push_back(n);
					}
				}
				res.regions.swap(next);
			}

			return res;
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
	};


//	template<typename T, unsigned Dims = 2>
//	class Grid : public core::data_item<GridFragment<T,Dims>> {
//
//	};

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
