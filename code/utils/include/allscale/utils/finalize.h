#pragma once

namespace allscale {
namespace utils {

	namespace detail {

		/**
		 * A utility to implement operations on scope-exit.
		 */
		template<typename Op>
		class finalizer {
			Op op;
		public:

			finalizer(const Op& op) : op(op) {}
			finalizer(const finalizer&) = delete;
			finalizer(finalizer&&) = default;

			~finalizer() { op(); }

			finalizer& operator=(const finalizer&) = delete;
			finalizer& operator=(finalizer&&) = default;
		};

	}

	/**
	 * Creates an object which will trigger the provided operation on destruction.
	 */
	template<typename Op>
	detail::finalizer<Op> run_finally(const Op& op) {
		return detail::finalizer<Op>{op};
	}

} // end namespace utils
} // end namespace allscale
