#pragma once

#include <cstring>
#include <type_traits>

#include "allscale/utils/serializer.h"
#include "allscale/utils/vector.h"

namespace allscale {
namespace utils {

	template<typename Cell, size_t ... size>
	struct StaticGrid;

	template<typename Cell, size_t a, size_t ... rest>
	struct StaticGrid<Cell,a,rest...> {
		using data_type = std::array<StaticGrid<Cell,rest...>,a>;
		using addr_type = utils::Vector<int64_t,sizeof...(rest)+1>;
		data_type data;

		StaticGrid& operator=(const StaticGrid& other) {
			if (this == &other) return *this;
			assignInternal<Cell>(other);
			return *this;
		}

	private:

		template<typename T>
		typename std::enable_if<std::is_trivially_copyable<T>::value,void>::type
		assignInternal(const StaticGrid& other) {
			std::memcpy(&data,&other.data,sizeof(data_type));
		}

		template<typename T>
		typename std::enable_if<!std::is_trivially_copyable<T>::value,void>::type
		assignInternal(const StaticGrid& other) {
			data = other.data;
		}

	public:

		Cell& operator[](const addr_type& addr) {
			std::array<int64_t, sizeof...(rest)+1> index = addr;
			std::array<int64_t, sizeof...(rest)> nested;
			std::copy(index.rbegin(), index.rend()-1, nested.begin());
			return data[index.front()][nested];
		}

		const Cell& operator[](const addr_type& addr) const {
			std::array<int64_t, sizeof...(rest)+1> index = addr;
			std::array<int64_t, sizeof...(rest)> nested;
			std::copy(index.rbegin(), index.rend()-1, nested.begin());
			return data[index.front()][nested];
		}

		utils::Vector<std::size_t, sizeof...(rest)+1> size() const {
			return { a, rest... };
		}

		template<typename Lambda>
		void forEach(const Lambda& lambda) const {
			for(const auto& cur : data) {
				cur.forEach(lambda);
			}
		}

		template<typename Lambda>
		void forEach(const Lambda& lambda) {
			for(auto& cur : data) {
				cur.forEach(lambda);
			}
		}

		void store(utils::ArchiveWriter& writer) const {
			for(const auto& e : data) {
				writer.write(e);
			}
		}

		static StaticGrid load(utils::ArchiveReader& reader) {
			StaticGrid grid;
			for(auto& e : grid.data) {
				e = reader.read<typename data_type::value_type>();
			}
			return grid;
		}

	};

	template<typename Cell>
	struct StaticGrid<Cell> {
		using data_type = Cell;
		using addr_type = utils::Vector<int64_t, 0>;

		data_type data;

		StaticGrid& operator=(const StaticGrid& other) {
			if (this == &other) return *this;
			assignInternal<Cell>(other);
			return *this;
		}

	private:

		template<typename T>
		typename std::enable_if<std::is_trivially_copyable<T>::value,void>::type
		assignInternal(const StaticGrid& other) {
			std::memcpy(&data,&other.data,sizeof(data_type));
		}

		template<typename T>
		typename std::enable_if<!std::is_trivially_copyable<T>::value,void>::type
		assignInternal(const StaticGrid& other) {
			data = other.data;
		}

	public:

		Cell& operator[](const addr_type&) {
			return data;
		}

		const Cell& operator[](const addr_type&) const {
			return data;
		}

		std::size_t size() const {
			return 1;
		}

		template<typename Lambda>
		void forEach(const Lambda& lambda) const {
			lambda(data);
		}

		template<typename Lambda>
		void forEach(const Lambda& lambda) {
			lambda(data);
		}

		void store(utils::ArchiveWriter& writer) const {
			writer.write(data);
		}

		static StaticGrid load(utils::ArchiveReader& reader) {
			StaticGrid grid;
			grid.data = std::move(reader.read<data_type>());
			return grid;
		}

	};

} // end utils
} // end namespace allscale
