#pragma once

#include <cstdint>
#include <ostream>

#include "allscale/utils/assert.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	/**
	 * An identifier of work items. Each work item is either a root-work-item,
	 * created by an initial prec call, or a child work item created through the
	 * splitting of a parent work item. The identifier is tracing this parent-child
	 * relationship.
	 *
	 * E.g. parent work item ID:
	 *
	 * 			T-12.0.1.0.1
	 *
	 * 		child work items:
	 *
	 * 			T-12.0.1.0.1.0 and WI-12.0.1.0.1.1
	 *
	 */
	class TaskID {

		std::uint64_t id;
		std::uint64_t path;
		std::uint8_t length;

	public:

		TaskID() = default;

		TaskID(std::uint64_t id)
			: id(id),path(0),length(0) {}


		// -- observers --

		std::uint64_t getRootID() const {
			return id;
		}

		std::uint64_t getDepth() const {
			return length;
		}

		// -- utility functions --

		bool operator==(const TaskID& other) const {
			return id == other.id && path == other.path && length == other.length;
		}

		bool operator!=(const TaskID& other) const {
			return !(*this == other);
		}

		bool operator<(const TaskID& other) const {
			// check id
			if (id < other.id) return true;
			if (id > other.id) return false;

			// get common prefix length
			auto min_len = std::min(length, other.length);

			auto pA = path >> (length - min_len);
			auto pB = other.path >> (other.length - min_len);

			// lexicographical compare
			if (pA == pB) {
				return length < other.length;
			}

			// compare prefix comparison
			return pA < pB;
		}

		bool isParentOf(const TaskID& child) const {
			return id == child.id && length < child.length && (path == child.path >> (child.length-length));
		}

		TaskID getLeftChild() const {
			assert_lt(length,sizeof(path)*8);
			auto res = *this;
			res.path = res.path << 1;
			++res.length;
			return res;
		}

		TaskID getRightChild() const {
			auto res = getLeftChild();
			res.path = res.path + 1;
			return res;
		}

		friend std::ostream& operator<<(std::ostream& out, const TaskID& id) {
			out << "T-" << id.id;

			std::uint64_t p = id.path;
			for(std::uint8_t i =0; i<id.length; ++i) {
				out << "." << (p%2);
				p = p >> 1;
			}

			return out;
		}

	};


} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
