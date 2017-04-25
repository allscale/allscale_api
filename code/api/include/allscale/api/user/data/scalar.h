#pragma once

#include <cstring>
#include <memory>

#include "allscale/api/core/data.h"

#include "allscale/utils/assert.h"
#include "allscale/utils/printer/join.h"
#include "allscale/utils/large_array.h"
#include "allscale/utils/vector.h"

namespace allscale {
namespace api {
namespace user {
namespace data {

	// ---------------------------------------------------------
	//						Declarations
	// ---------------------------------------------------------


	/**
	 * A data item wrapper for scalar values.
	 */
	template<typename T>
	class Scalar;


	// ---------------------------------------------------------
	//						Definitions
	// ---------------------------------------------------------


	namespace detail {

		/**
		 * The type utilized to address regions of scalar data items. The region
		 * defines the unit region of either being present or not.
		 */
		class ScalarRegion {

			// indicating whether the value is present or not
			bool flag;

		public:

			ScalarRegion() = default;

			ScalarRegion(bool value) : flag(value) {}

			bool operator==(const ScalarRegion& other) const {
				return flag == other.flag;
			}

			bool operator!=(const ScalarRegion& other) const {
				return flag != other.flag;
			}

			/**
			 * The empty check returns true if the value is not present.
			 */
			bool empty() const {
				return !flag;
			}

			static ScalarRegion merge(const ScalarRegion& a, const ScalarRegion& b) {
				return { a.flag || b.flag };
			}

			static ScalarRegion intersect(const ScalarRegion& a, const ScalarRegion& b) {
				return { a.flag && b.flag };
			}

			static ScalarRegion difference(const ScalarRegion& a, const ScalarRegion& b) {
				return  a.flag && !b.flag;
			}

			/**
			 * An operator to load an instance of this range from the given archive.
			 */
			static ScalarRegion load(utils::Archive&) {
				assert_not_implemented();
				return {};
			}

			/**
			 * An operator to store an instance of this range into the given archive.
			 */
			void store(utils::Archive&) const {
				assert_not_implemented();
				// nothing so far
			}

			friend std::ostream& operator<<(std::ostream& out, const ScalarRegion& region) {
				return out << (region.flag ? "+" : "-");
			}

		};


		/**
		 * A scalar data item fragment provides the capability of maintaining a copy of
		 * the covered scalar value.
		 */
		template<typename T>
		class ScalarFragment {

			// the stored value
			T value;

			// the region covered -- thus, indicating whether the value is present or not
			ScalarRegion covered;

			friend class Scalar<T>;

		public:

			using region_type = ScalarRegion;
			using shared_data_type = core::no_shared_data;
			using facade_type = Scalar<T>;

			ScalarFragment(const core::no_shared_data&, const ScalarRegion& region)
				: covered(region) {}

			const ScalarRegion& getCoveredRegion() const {
				return covered;
			}

			void resize(const ScalarRegion& newSize) {
				covered = newSize;
			}

			void insert(const ScalarFragment& f, const ScalarRegion& region) {
				assert_false(covered.empty());
				if (region.empty()) return;
				value = f.value;
			}

			void save(utils::Archive&, const ScalarRegion&) const {
				assert_not_implemented();
			}

			void load(utils::Archive&) {
				assert_not_implemented();
			}

			Scalar<T> mask() {
				return Scalar<T>(*this);
			}

		};

	}


	template<typename T>
	class Scalar : public core::data_item<detail::ScalarFragment<T>> {

		friend class detail::ScalarFragment<T>;

		std::unique_ptr<detail::ScalarFragment<T>> owned;

		detail::ScalarFragment<T>* base;

		Scalar(detail::ScalarFragment<T>& fragment)
			: base(&fragment) {}

	public:

		Scalar(const T& value = T())
			: owned(std::make_unique<detail::ScalarFragment<T>>(value)), base(owned.get()) {}

		T& get() {
			return base->value;
		}

		const T& get() const {
			return base->value;
		}

		void set(const T& newValue) {
			base->value = newValue;
		}

	};

} // end namespace data
} // end namespace user
} // end namespace api
} // end namespace allscale
