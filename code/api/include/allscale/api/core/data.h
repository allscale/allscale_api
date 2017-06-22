#pragma once

#include <type_traits>

#include "allscale/utils/concepts.h"
#include "allscale/utils/serializer.h"

namespace allscale {
namespace api {
namespace core {

	// ---------------------------------------------------------------------------------
	//									  Regions
	// ---------------------------------------------------------------------------------


	template<typename R, typename _ = void>
	struct is_region : public std::false_type {};

	template<typename R>
	struct is_region<R,typename std::enable_if<

			// regions have to be values (constructible, assignable, comparable)
			utils::is_value<R>::value &&

			// regions have to be serializable
			utils::is_serializable<R>::value &&

			// there has to be an emptiness check
			std::is_same<decltype((bool (R::*)(void) const)(&R::empty)), bool (R::*)(void) const>::value &&

			// there has to be an union operation
			std::is_same<decltype((R (*)(const R&, const R&))(&R::merge)), R(*)(const R&, const R&)>::value &&

			// there has to be an intersection operation
			std::is_same<decltype((R(*)(const R&, const R&))(&R::intersect)), R(*)(const R&, const R&)>::value &&

			// there has to be a set difference operation
			std::is_same<decltype((R(*)(const R&, const R&))(&R::difference)), R(*)(const R&, const R&)>::value,

		void>::type> : public std::true_type {};




	// ---------------------------------------------------------------------------------
	//									 Fragments
	// ---------------------------------------------------------------------------------


	
	template<typename F, typename _ = void>
	struct is_fragment : public std::false_type {};

	template<typename F>
	struct is_fragment<F, typename std::enable_if<

		// fragment needs to expose a region type
		is_region<typename F::region_type>::value &&

		// fragments need to be constructible for a given region
		std::is_same<decltype(F(std::declval<const typename F::shared_data_type&>(), std::declval<const typename F::region_type&>())), F>::value &&

		// fragments need to be destructible
		std::is_destructible<F>::value &&

		// the region covered by the fragment has to be obtainable
		std::is_same<decltype((void (F::*)(utils::Archive&) const)(&F::getCoveredRegion)), void (F::*)(utils::Archive&) const>::value &&

		// there has to be a resize operator
		std::is_same<decltype((void (F::*)(const typename F::region_type&))(&F::resize)), void (F::*)(const typename F::region_type&)>::value &&

		// there is an insert operator importing data from an existing fragment
		std::is_same<decltype((void (F::*)(const F&, const typename F::region_type&))(&F::insert)), void (F::*)(const F&, const typename F::region_type&)>::value &&

		// there is a extract operator extracting a region of data from the present fragment
		std::is_same<decltype((void (F::*)(utils::ArchiveWriter&, const typename F::region_type&) const)(&F::extract)), void (F::*)(utils::ArchiveWriter&, const typename F::region_type&) const>::value &&

		// there is a insert operator, importing previously extracted data into this fragment
		std::is_same<decltype((void (F::*)(utils::ArchiveReader&))(&F::insert)), void (F::*)(utils::ArchiveReader&)>::value &&

		// can be concerted into a facade
		std::is_same<decltype((typename F::facade_type (F::*)(void))(&F::mask)), typename F::facade_type(F::*)(void)>::value,

		void>::type> : public std::true_type{};





	// ---------------------------------------------------------------------------------
	//									SharedData
	// ---------------------------------------------------------------------------------

	template<typename S, typename T = void>
	struct is_shared_data { };

	template<typename S>
	struct is_shared_data<S> {

	private:

		template<typename T>
		static constexpr auto check(T*)
			-> 
			typename std::conditional<
					// needs to be destructible
					std::is_destructible<S>::value &&
					// there must be a static load fuction
					std::is_same<decltype(T::load(std::declval<utils::Archive&>())), T>::value &&
					// there must be a save member function
					std::is_same<decltype(std::declval<const T&>().save(std::declval<utils::Archive&>())),void>::value,
				std::true_type,
				std::false_type
			>::type;

		template<typename>
		static constexpr std::false_type check(...);

		typedef decltype(check<S>(0)) type;

	public:
		
		enum { value = type::value };

	};



	// ---------------------------------------------------------------------------------
	//									  Facade
	// ---------------------------------------------------------------------------------


	template<typename F, typename _ = void>
	struct is_facade : public std::false_type {};

	template<typename F>
	struct is_facade<F,typename std::enable_if<

			// facade need to be constructible for a given region
			std::is_same<decltype(F(std::declval<const typename F::region_type&>())),F>::value &&

			// fragments need to be destructible
			std::is_destructible<F>::value,

		void>::type> : public std::true_type {};


	// ---------------------------------------------------------------------------------
	//									  Data Items
	// ---------------------------------------------------------------------------------


	template<typename D, typename _ = void>
	struct is_data_item : public std::false_type {};

	template<typename D>
	struct is_data_item<D,typename std::enable_if<
			std::is_same<D,typename D::facade_type>::value &&
			is_fragment<typename D::fragment_type>::value &&
			is_shared_data<typename D::shared_data_type>::value,
		void>::type> : public std::true_type {};


	template<
		typename Fragment
	>
	struct data_item {

		// make sure the region type is satisfying the concept
		static_assert(is_region<typename Fragment::region_type>::value, "Region type must fit region concept!");
		static_assert(is_fragment<Fragment>::value, "Fragment type must fit fragment concept!");
		static_assert(is_shared_data<typename Fragment::shared_data_type>::value, "Shared data type must fit shared data concept!");

		using fragment_type = Fragment;
		using region_type = typename Fragment::region_type;
		using facade_type = typename Fragment::facade_type;
		using shared_data_type = typename Fragment::shared_data_type;
	};


	// ---------------------------------------------------------------------------------
	//									  Utilities
	// ---------------------------------------------------------------------------------


	/**
	 * A generic utility to compute whether a region a is covering a sub-set of a region b.
	 */
	template<typename R>
	typename std::enable_if<is_region<R>::value,bool>::type
	isSubRegion(const R& a, const R& b) {
		return R::difference(a,b).empty();
	}

	/**
	 * A default implementation of shared data for data items that do not need shared any shared data.
	 */
	struct no_shared_data {

		void save(utils::Archive&) const {
			// nothing to do
		}

		static no_shared_data load(utils::Archive&) {
			return no_shared_data();
		}

	};

	// make sure the no_shared_data is a shared data instance
	static_assert(is_shared_data<no_shared_data>::value, "no_shared_data type does not fulfill shared data concept!");

} // end namespace core
} // end namespace api
} // end namespace allscale
