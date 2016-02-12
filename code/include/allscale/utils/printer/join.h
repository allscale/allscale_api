#pragma once

namespace allscale {
namespace utils {


	namespace detail {

		template<typename Sep, typename Iter>
		class joinable {

			Iter begin;
			Iter end;
			Sep sep;

		public:

			joinable(const Iter& begin, const Iter& end, const Sep& sep)
				: begin(begin), end(end), sep(sep) {}

			friend
			std::ostream& operator<<(std::ostream& out, const joinable& j) {
				if (j.begin == j.end) return out;
				Iter cur = j.begin;
				out << *cur;
				cur++;
				while(cur != j.end) {
					out << j.sep;
					out << *cur;
					cur++;
				}
				return out;
			}

		};

	}


	template<typename Iter>
	detail::joinable<const char*,Iter> join(const char* sep, const Iter& begin, const Iter& end) {
		return detail::joinable<const char*,Iter>(begin,end,sep);
	}

	template<typename Iter>
	detail::joinable<std::string,Iter> join(const std::string& sep, const Iter& begin, const Iter& end) {
		return detail::joinable<std::string,Iter>(begin,end,sep);
	}

	template<typename Sep, typename Container, typename Iter = typename Container::const_iterator>
	auto join(const Sep& sep, const Container& c) -> decltype(join(sep, c.cbegin(), c.cend())) {
		return join(sep, c.cbegin(), c.cend());
	}


} // end namespace utils
} // end namespace allscale
