#pragma once

#include <utility>

#include "allscale/utils/assert.h"

#include "allscale/api/core/prec.h"

namespace allscale {
namespace api {
namespace user {



	// ---------------------------------------------------------------------------------------------
	//									    Declarations
	// ---------------------------------------------------------------------------------------------


	/**
	 * A simple job wrapper processing a given task asynchronously. The task
	 * is wrapped to a simple recursion where there is a single base
	 * case step.
	 *
	 * @tparam Action the type of action
	 * @param action the action to be processed
	 * @return a treeture providing a reference the the result
	 */
	template<typename Action>
	core::treeture<std::result_of_t<Action()>> async(const Action& action);




	// ---------------------------------------------------------------------------------------------
	//									    Definitions
	// ---------------------------------------------------------------------------------------------


	template<typename Action>
	core::treeture<std::result_of_t<Action()>> async(const Action& action) {
		struct empty {};
		return core::prec(
			[](empty){ return true; },
			[=](empty){
				return action();
			},
			[=](empty,const auto&){
				assert_fail() << "Should not be reached!";
				return action();
			}
		)(empty());
	}


} // end namespace user
} // end namespace api
} // end namespace allscale
