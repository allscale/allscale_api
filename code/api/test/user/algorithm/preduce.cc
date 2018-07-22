#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "allscale/api/user/algorithm/preduce.h"

namespace allscale {
namespace api {
namespace user {
namespace algorithm {

	TEST(Ops, Reduce) {
		auto plus = [](int a, int b) { return a + b; };

		std::vector<int> v = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };
		EXPECT_EQ(351, preduce(v, plus).get());

		std::vector<int> e = { };
		EXPECT_EQ(0, preduce(e, plus).get());

		auto concat = [](std::string a, std::string b) { return a + b; };
		std::vector<std::string> s = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n",
				"o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};
		std::string res = preduce(s, concat).get();
		EXPECT_EQ(26, res.size());
		for(std::string cur : s) {
			EXPECT_NE(std::string::npos, res.find(cur));
		}
	}


	TEST(Ops, FoldReduce) {
		const int N = 10;

		std::vector<int> data;
		for(int i = 0; i<N; i++) {
			data.push_back(1);
		}

		auto fold = [](int i, int& s) { s += i + 1; };
		auto reduce = [](int a, int b) { return a + b; };
		auto init = []() { return 0; };

		auto res = preduce(data, fold, reduce, init).get();

		EXPECT_EQ(N*2,res);

	}

	TEST(Ops, FindMaxAndAvg) {
		const int N = 10;

		struct Data {
			int max;
			int sum;
			int num;
		};

		std::vector<int> data;
		for(int i = 0; i<N; i++) {
			data.push_back(i);
		}

		auto fold = [](int i, Data& s) {
			s.max = std::max(s.max,i);
			s.sum += i,
			s.num++;
		};

		auto reduce = [](Data a, Data b) {
			return Data{
				std::max(a.max, b.max),
				a.sum + b.sum,
				a.num + b.num
			};
		};

		auto init = []() { return Data{0,0,0}; };

		auto res = preduce(data, fold, reduce, init).get();

		int max = res.max;
		double avg = (double)res.sum / res.num;

		EXPECT_EQ(N-1,max);
		EXPECT_EQ((double)(N-1)/2, avg);

	}

	TEST(Ops, MapReduceDataFilter) {
		const int N = 1000000;

		std::vector<int> data;
		for(int i = 0; i<N; i++) {
			data.push_back(i);
		}

		struct Result {
			std::vector<int> even;
			std::vector<int> odd;
		};

		auto fold = [](int i, Result& r) {
			if (i % 2 == 0) r.even.push_back(i);
			else            r.odd.push_back(i);
		};


		// merging of results
		auto reduce = [](Result a, Result b) {

			std::vector<int> even(std::move(a.even));
			even.insert(even.end(),b.even.begin(),b.even.end());

			std::vector<int> odd(std::move(a.odd));
			odd.insert(odd.end(),b.odd.begin(),b.odd.end());

			return Result{even,odd};
		};

		auto init = []() { return Result(); };

		auto res = preduce(data, fold, reduce, init).get();


		std::sort(res.even.begin(),res.even.end());
		std::sort(res.odd.begin(),res.odd.end());

		for(int i=0; i<N/2; ++i) {
			EXPECT_EQ(2*i,res.even[i]);
			EXPECT_EQ(2*i+1,res.odd[i]);
		}
	}

	TEST(Ops, MapReduceAlphabet) {
		std::vector<int> characters;
		for(int i = 97; i < 123; ++i)
			characters.push_back(i);

		auto fold = [](int i, std::vector<char>& acc) {
			acc.push_back((char)i);
		};

		auto reduce = [](std::string a, std::string b) {
			return a + b;
		};

		auto init = []() { return std::vector<char>(); };
		auto exit = [](std::vector<char> vec) { return std::string(vec.begin(), vec.end()); };

		auto res = preduce(characters, fold, reduce, init, exit).get();

		EXPECT_EQ(26, res.size());
		for(int cur : characters) {
			EXPECT_NE(std::string::npos, res.find((char)cur));
		}
	}

	TEST(Ops, MapReduce2D) {
		const int N = 10;

		std::array<int, 2> start{{0, 0}};
		std::array<int, 2> end{{N, N}};

		std::vector<int> data;
		for(int i = 0; i<N; i++) {
			for(int i = 0; i<N; i++) {
				data.push_back(1);
			}
		}

		auto fold = [data](std::array<int,2> i, int& s) { s += data[i[0]*10 + i[1]]; };
		auto reduce = [](int a, int b) { return a + b; };
		auto init = []() { return 0; };
		auto exit = [](int i) { return i; };

		auto res = preduce(start, end, fold, reduce, init, exit).get();

		EXPECT_EQ(N*N, res);
	}


	TEST(Ops, MapReduce3D) {
		const int X = 10;
		// only declared static to silence MSVC errors...
		static const int Y = 5;
		static const int Z = 7;

		std::array<int,3> start{{0,0,1}};
		std::array<int,3> end{{X,Y,Z}};

		int cnt = 0;

		std::vector<int> data;
		for(int i = 0; i<X; i++) {
			for(int j = 0; j<Y; j++) {
				for(int k = 0; k<Z; k++) {
					data.push_back(i+j+k);
					if(k>0) cnt += data.back();
				}
			}
		}



		auto fold = [data](std::array<int,3> i, int& s) { s += data[i[0]*Y*Z + i[1]*Z + i[2]]; };
		auto reduce = [](double a, double b) { return a + b; };
		auto init = []() { return 0; };
		auto exit = [](int i) { return 0.1*i; };

		auto res = preduce(start, end, fold, reduce, init, exit).get();

		EXPECT_EQ(cnt/10, res);
	}

} // end namespace algorithm
} // end namespace user
} // end namespace api
} // end namespace allscale
