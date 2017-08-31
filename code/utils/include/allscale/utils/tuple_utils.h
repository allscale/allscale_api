#include <tuple>
#include <utility>

namespace allscale {
namespace utils {

	namespace detail {

		template<std::size_t Pos>
		struct tuple_for_each_helper {
			template<typename Op, typename ... Args>
			void operator()(const Op& op, std::tuple<Args...>& tuple) {
				tuple_for_each_helper<Pos-1>()(op,tuple);
				op(std::get<Pos-1>(tuple));
			}
			template<typename Op, typename ... Args>
			void operator()(const Op& op, const std::tuple<Args...>& tuple) {
				tuple_for_each_helper<Pos-1>()(op,tuple);
				op(std::get<Pos-1>(tuple));
			}
		};

		template<>
		struct tuple_for_each_helper<0> {
			template<typename Op, typename ... Args>
			void operator()(const Op&, const std::tuple<Args...>&) {
				// nothing
			}
		};

	}

	/**
	 * A utility to apply an operator on all elements of a tuple in order.
	 *
	 * @param tuple the (mutable) tuple
	 * @param op the operator to be applied
	 */
	template<typename Op, typename ... Args>
	void forEach(std::tuple<Args...>& tuple, const Op& op) {
		detail::tuple_for_each_helper<sizeof...(Args)>()(op,tuple);
	}

	/**
	 * A utility to apply an operator on all elements of a tuple in order.
	 *
	 * @param tuple the (constant) tuple
	 * @param op the operator to be applied
	 */
	template<typename Op, typename ... Args>
	void forEach(const std::tuple<Args...>& tuple, const Op& op) {
		detail::tuple_for_each_helper<sizeof...(Args)>()(op,tuple);
	}

} // end namespace utils
} // end namespace allscale

namespace std {

	template<typename ... Elements>
	std::ostream& operator<<(std::ostream& out, const std::tuple<Elements...>& tuple) {
		out << "(";
		std::size_t count = 0;
		allscale::utils::forEach(tuple,[&](const auto& cur) {
			out << cur;
			count++;
			if (count != sizeof...(Elements)) out << ",";
		});
		return out << ")";
	}

}
