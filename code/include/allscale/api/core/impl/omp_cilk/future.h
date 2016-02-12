#pragma once

#include <functional>
#include <type_traits>
#include <vector>

#ifdef __cilk
    #include <cilk/cilk.h>
    #define parallel_for cilk_for
#elif _OPENMP
    #define parallel_for _Pragma ("omp parallel for") for
#else
    #define parallel_for for
#endif

namespace allscale {
namespace api {
namespace core {
inline namespace omp_cilk {

	template<typename T>
	using Task = std::function<T()>;

	namespace internal {

		// the kinds of composed futures
		enum class Kind {
			Atomic,
			Sequential,
			Parallel
		};

		template<typename Future, typename Task>
		class FutureBase {

			// the kind of composed future represented
			Kind kind;

			// a flag whether it is complete (if composed, a cache)
			mutable bool done;

			// for atomic tasks: the associated task
			Task work;

			// for composed tasks: the list of sub-tasks
			std::vector<Future> subTasks;

		protected:

			FutureBase(const Task& work)
				: kind(Kind::Atomic), done(false), work(work) {}

			template<typename ... Subs>
			FutureBase(Kind kind, Subs&& ... subs)
				: kind(kind), done(false), subTasks(sizeof...(subs)) {
				fill(0,std::move(subs)...);
			}

			void fill(unsigned) {}

			template<typename ... Subs>
			void fill(unsigned i, Future&& first, Subs&&  ... rest) {
				subTasks[i] = std::move(first);
				fill(i+1,std::move(rest)...);
			}

		public:

			FutureBase() : kind(Kind::Atomic), done(true) {}

			FutureBase(const FutureBase&) = delete;

			FutureBase(FutureBase&& other)
				: kind(other.kind), done(other.done), work(std::move(other.work)), subTasks(std::move(other.subTasks)) {
				other.done = true;
			}

			~FutureBase() {
				wait();
			}

			FutureBase& operator=(const FutureBase& other) = delete;

			FutureBase& operator=(FutureBase&& other) {
				return *new (this) FutureBase(std::move(other));
			}

			bool isDone() const {
				if (done) return true;
				if (isAtom()) return false;
				for(const auto& cur : subTasks) {
					if (!cur.isDone()) return false;
				}

				// notify implementation on completion
				static_cast<const Future&>(*this).completed();

				// done
				done = true;
				return true;
			}

			bool isAtom() const {
				return kind == Kind::Atomic;
			}

			bool isSequence() const {
				return kind == Kind::Sequential;
			}

			bool isParallel() const {
				return kind == Kind::Parallel;
			}

			bool isComposed() const {
				return !isAtom();
			}

			const std::vector<Future>& getSubTasks() const {
				return subTasks;
			}

			void wait() const {
				if (done) return;
				if (kind == Kind::Atomic) {
					if(!done) {
						work();
					}
				} else if (kind == Kind::Sequential) {
					for(auto& cur : subTasks) {
						cur.wait();
					}
				} else {
					parallel_for(auto it = subTasks.begin(); it < subTasks.end(); ++it) {
						it->wait();
					}
				}

				// notify implementation on completion
				static_cast<const Future&>(*this).completed();

				done = true;
			}

		};

	}

	// ---------------------------------------------------------------------------------------------
	//											Futures
	// ---------------------------------------------------------------------------------------------

	/**
	 * The general future implementation.
	 */
	template<typename T>
	class Future : public internal::FutureBase<Future<T>,Task<T>> {

		using super = internal::FutureBase<Future<T>,Task<T>>;
		using aggregator_t = T(*)(const std::vector<Future<T>>&);

		mutable T value;

		aggregator_t aggregator;

	private:

		Future(const Task<T>& work)
			: super(work) {}

		template<typename ... Subs>
		Future(internal::Kind kind, const aggregator_t& aggregator, Subs&& ... subs)
			: super(kind, subs...), aggregator(aggregator) {}

	public:

		Future(const T& value = T()) : super() {
			this->value = value;
		}

		Future(const Future&) = delete;

		Future(Future&& other)
			: super(std::move(other)) {
			// TODO: fix this race condition
			value = other.value;
			aggregator = other.aggregator;
		}

		~Future() {
			aggregator = nullptr;
		}

		Future& operator=(const Future& other) = delete;

		Future& operator=(Future&& other) {
			return *new (this) Future(std::move(other));
		}

		const T& get() const {
			this->wait();
			return value;
		}

	private:

		friend internal::FutureBase<Future,Task<T>>;

		void completed() const {
			if (aggregator) {
				value = aggregator(this->getSubTasks());
			}
		}

	public:

		// ---- factories ----

		template<typename S, typename R> friend
		Future<R> atom_internal(const S&);

		template<typename V, typename ... Subs> friend
		Future<V> aggregate(V(*)(const std::vector<Future<V>>&), Subs&& ...);

	};

	/**
	 * A specialized implementation of a void future.
	 */
	template<>
	class Future<void> : public internal::FutureBase<Future<void>,Task<void>> {

		using super = internal::FutureBase<Future<void>,Task<void>>;

		Future(const Task<void>& work)
			: super(work) {}

		template<typename ... Subs>
		Future(internal::Kind kind, Subs&& ... subs)
			: super(kind, subs...) {}

	public:

		Future() : super() {}

		Future(const Future&) = delete;

		Future(Future&& other)
			: super(std::move(other)) {}

		Future& operator=(const Future& other) = delete;

		Future& operator=(Future&& other) {
			return *new (this) Future(std::move(other));
		}

	private:

		friend internal::FutureBase<Future,Task<void>>;

		void completed() const {
			// nothing
		}

	public:

		// ---- factories ----

		template<typename S, typename R> friend
		Future<R> atom_internal(const S&);

		template<typename ... Subs> friend
		Future<void> par(Subs&& ...);

		template<typename ... Subs> friend
		Future<void> seq(Subs&& ...);

	};


	// ---------------------------------------------------------------------------------------------
	//											Factories
	// ---------------------------------------------------------------------------------------------

	// -- completed futures --

	template<typename T>
	Future<T> done(const T& value) {
		return Future<T>(value);
	}

	inline Future<void> done() {
		return Future<void>();
	}


	// -- atomic futures --

	template<typename T, typename R>
	Future<R> atom_internal(const T& task) {
		return Future<R>(task);
	}


	template<
		typename T,
		typename R = typename std::result_of<T()>::type
	>
	Future<R> atom(const T& task) {
		return atom_internal<T,R>(task);
	}


	// -- composed futures --

	template<typename V, typename ... Subs>
	Future<V> aggregate(V(* aggregator)(const std::vector<Future<V>>&), Subs&& ... subs) {
		return Future<V>(internal::Kind::Parallel, aggregator, subs...);
	}

	template<typename ... Subs>
	Future<void> par(Subs&& ... subs) {
		return Future<void>(internal::Kind::Parallel, subs...);
	}

	template<typename ... Subs>
	Future<void> seq(Subs&& ... subs) {
		return Future<void>(internal::Kind::Sequential, subs...);
	}


} // end namespace naive
} // end namespace core
} // end namespace api
} // end namespace allscale


