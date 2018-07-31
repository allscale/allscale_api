#pragma once

#include "allscale/utils/serializer.h"

#include <tuple>

#include "allscale/utils/tuple_utils.h"

namespace allscale {
namespace utils {

	/**
	 * Add support for serializing / de-serializing tuples of trivial element types.
	 */
	template<typename ... Args>
	struct is_trivially_serializable<std::tuple<Args...>, typename std::enable_if<all_trivially_serializable<Args...>::value>::type> : public std::true_type {};


	namespace detail {

		// a utility assisting in loading non-trivial tuples from stream

		template<std::size_t pos, typename ... Args>
		struct load_helper {

			using inner = load_helper<pos-1,Args...>;

			template<typename ... Cur>
			std::tuple<Args...> operator()(ArchiveReader& in, Cur&& ... cur) const {
				using cur_t = std::remove_reference_t<decltype(std::get<pos-1>(std::declval<std::tuple<Args...>>()))>;
				return inner{}(in,std::move(cur)...,in.read<cur_t>());
			}

		};

		template<typename ... Args>
		struct load_helper<0,Args...> {

			std::tuple<Args...> operator()(ArchiveReader&, Args&& ... args) const {
				return std::make_tuple(std::move(args)...);
			}

		};

	}


	/**
	 * Add support for serializing / de-serializing tuples of non-trivial element types.
	 */
	template<typename ... Args>
	struct serializer<std::tuple<Args...>,typename std::enable_if<
				all_serializable<Args...>::value && !all_trivially_serializable<Args...>::value,
			void>::type> {

		static std::tuple<Args...> load(ArchiveReader& reader) {
			return detail::load_helper<sizeof...(Args),Args...>{}(reader);
		}


		static void store(ArchiveWriter& writer, const std::tuple<Args...>& value) {
			forEach(value,[&](const auto& cur){
				writer.write(cur);
			});
		}
	};


} // end namespace utils
} // end namespace allscale
