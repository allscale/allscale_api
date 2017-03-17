#include <gtest/gtest.h>


#include "allscale/api/core/treeture.h"
#include "allscale/api/user/arithmetic.h"

#include "allscale/utils/string_utils.h"

#include "allscale/api/core/impl/sequential/treeture.h"
#include "allscale/api/core/impl/reference/treeture.h"

namespace allscale {
namespace api {
namespace user {

	using namespace core;

	namespace {

		template<typename Op, typename A, typename B, typename C>
		void check(const Op& op, const A& a, const B& b, const C& res) {

			// check all allowed combinations
			EXPECT_EQ(res, op(done(a),done(b)).get());

			EXPECT_EQ(res, op(done(a),impl::sequential::done(b)).get());
			EXPECT_EQ(res, op(done(a),impl::reference::done(b)).get());

			EXPECT_EQ(res, op(impl::sequential::done(a),done(b)).get());
			EXPECT_EQ(res, op(impl::sequential::done(a),impl::sequential::done(b)).get());

			EXPECT_EQ(res, op(impl::reference::done(a),done(b)).get());
			EXPECT_EQ(res, op(impl::reference::done(a),impl::reference::done(b)).get());

		}

	}


	TEST(Arithmetic,add) {

		auto op = [](auto&& a, auto&& b) {
			return add(std::move(a),std::move(b));
		};

		// check with integers
		check(op,8,4,12);
		check(op,10,8,18);

		// and doubles
		check(op,1.0,2.0,3.0);

		// and strings
		check(op,std::string("ab"),std::string("cd"),std::string("abcd"));
	}


	TEST(Arithmetic,sub) {

		auto op = [](auto&& a, auto&& b) {
			return sub(std::move(a),std::move(b));
		};

		// check with integers
		check(op,8,2,6);

		// and doubles
		check(op,3.0,2.0,1.0);
	}

	TEST(Arithmetic,mul) {

		auto op = [](auto&& a, auto&& b) {
			return mul(std::move(a),std::move(b));
		};

		// check with integers
		check(op,8,2,16);

		// and doubles
		check(op,3.0,2.0,6.0);
	}


	TEST(Arithmetic,min) {

		auto op = [](auto&& a, auto&& b) {
			return min(std::move(a),std::move(b));
		};

		// check with integers
		check(op,8,2,2);

		// and doubles
		check(op,3.0,2.0,2.0);
	}

	TEST(Arithmetic,max) {

		auto op = [](auto&& a, auto&& b) {
			return max(std::move(a),std::move(b));
		};

		// check with integers
		check(op,8,2,8);

		// and doubles
		check(op,3.0,2.0,3.0);
	}

} // end namespace user
} // end namespace api
} // end namespace allscale
