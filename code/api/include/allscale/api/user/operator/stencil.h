#pragma once

#include "allscale/api/user/operator/pfor.h"
#include "allscale/api/user/operator/internal/operation_reference.h"

namespace allscale {
namespace api {
namespace user {


	// ---------------------------------------------------------------------------------------------
	//									    Declarations
	// ---------------------------------------------------------------------------------------------


	class stencil_reference;

	template<typename Container, typename Update>
	stencil_reference stencil(Container& res, const Update& update);



	// ---------------------------------------------------------------------------------------------
	//									    Definitions
	// ---------------------------------------------------------------------------------------------



	class stencil_reference : public internal::operation_reference {

	public:

		// inherit all constructors
		using operation_reference::operation_reference;

	};

	template<typename Container, typename Update>
	stencil_reference stencil(Container& a, int steps, const Update& update) {

		// iterative implementation
		Container b = a;

		Container* x = &a;
		Container* y = &b;

		detail::loop_reference<std::size_t> ref;

		for(int t=0; t<steps; t++) {

			// prove-of-concept sequential implementation
			ref = pfor(std::size_t(0),a.size(),[x,y,t,update](std::size_t i){
				(*y)[i] = update(t,i,*x);
			}, neighborhood_sync(ref));

			// swap buffers
			std::swap(x,y);
		}

		// fix result in a
		if (x != &a) {
			// move final data to the original container
			ref = pfor(std::size_t(0),a.size(),[x,&a,update](std::size_t i){
				a[i] = (*x)[i];
			}, neighborhood_sync(ref));

		}

		// wait for the task completion
		ref.wait();

		// done
		return stencil_reference();

	}



} // end namespace user
} // end namespace api
} // end namespace allscale
