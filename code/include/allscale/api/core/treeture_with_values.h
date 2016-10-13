#pragma once

#include <memory>
#include <type_traits>

namespace allscale {
namespace api {
namespace core {

	// ---------------------------------------------------------------------------------------------
	//											  Tasks
	// ---------------------------------------------------------------------------------------------



	template<typename T>
	struct to_value_type {
		using type = T;
	};

	template<>
	struct to_value_type<void> {
		struct empty {};
		using type = empty;
	};




	// the RT's interface to a task
	class TaskBase {
	public:
		virtual ~TaskBase(){};
		virtual void process() =0;
		virtual void split() =0;
	};


	// the treeture's interface to a task
	template<typename T>
	class Task : public TaskBase {

		bool done;
		T res;

	protected:

		Task() : done(false) {};
		Task(const T& res) : done(true), res(res) {}
		Task(T&& res) : done(true), res(res) {}

	public:

		T get() {
			// check computation state
			if (!done) {
				// trigger the execution
				process();
				// mark as done
				done = true;
			}
			// return result
			return res;
		}

	};

	template<typename T>
	using TaskRef = std::shared_ptr<Task<T>>;

	// a task who's value is given immediatly
	template<typename T>
	class ImmediateTask : public Task<T> {
	public:
		ImmediateTask(const T& res) : Task<T>(res) {}
		ImmediateTask(T&& res) : Task<T>(res) {}
		void process() override {}
		void split() override {}
	};



	template<typename T>
	class ComposedTaskBase : public Task<T> {

	protected:

		bool parallel;

	public:

		virtual ~ComposedTaskBase() {}

		virtual void process(T& res) =0;

	};

	template<typename T, typename A, typename B, typename C>
	class ComposedTask : public ComposedTaskBase<T> {

		static_assert(std::is_same<T,std::result_of_t<C(A,B)>>::value, "Invalid result type of merge operation.");

		TaskRef<A> left;
		TaskRef<B> right;

		C merge;

	public:

		T process() override {
			if (this->parallel) {
				// TODO: run in parallel
				auto a = left->get();
				auto b = right->get();
				return merge(a,b);
			} else {
				auto a = left->get();
				auto b = right->get();
				return merge(a,b);
			}
		}

	};

	// an actual task implementation
	template<
		typename Compute,
		typename Split,
		typename result_type = std::result_of_t<Compute()>
	>
	class ConcreteTask : public Task<result_type> {

		Compute compute;

		Split decompose;

		std::unique_ptr<ComposedTaskBase<result_type>> subTask;

	public:

		ConcreteTask(const Compute& c, const Split& s)
			: compute(c), decompose(s) {}

		void process() override {
			if (subTask) {
				// trigger computation of sub-tasks
				this->res = subTask->process();
				// delete sub-tasks
				subTask.release();
			} else {
				// compute this task in a single step
				this->res = compute();
			}
		}

		void split() override {
			// TODO: run the split operation
		}

	};




	// ---------------------------------------------------------------------------------------------
	//											Treetures
	// ---------------------------------------------------------------------------------------------


	template<typename T>
	class treeture {

		TaskRef<T> task;

	public:

		treeture(const T& t) : task(std::make_shared<ImmediateTask<T>>(t)) {}

		T get() {
			return task->get();
		}



	};

	template<>
	class treeture<void> {

		TaskRef<void> task;

	public:
		treeture() : task() {}

		void get() {}
	};


	// ---------------------------------------------------------------------------------------------
	//											Operators
	// ---------------------------------------------------------------------------------------------


	inline treeture<void> done() {
		return treeture<void>();
	}

	template<typename T>
	treeture<T> done(const T& t) {
		return treeture<T>(t);
	}

/*
	template<typename T>
	class Task;

	template<typename T, typename A = void, typename B = void>
	class treeture;

	namespace detail {

		template<typename T>
		class TaskBase {
		public:
			void wait() const {}
		};

		template<typename T, typename A, typename B>
		class treeture_base {

			enum class Kind {
				Atomic,
				Sequential,
				Parallel
			};

			Kind kind;

			mutable bool done;

			// child tasks (potential)
			std::unique_ptr<treeture<A>> left;
			std::unique_ptr<treeture<B>> right;

		protected:

			Task<T> task;

		public:

			void wait() const {
				task.wait();
			}

		};

	}

	template<typename T>
	class Task : public detail::TaskBase<T> {
		T value;
	public:
		T getResult() {
			return value;
		}
	};

	template<>
	class Task<void> : public detail::TaskBase<void> {
	};


	template<typename T, typename A, typename B>
	class treeture : public detail::treeture_base<T,A,B> {

	public:

		// --- Constructors ---

		treeture() {}

		treeture(const treeture&) = delete;

		treeture(treeture&&) {}


		// --- Operators ---

		treeture& operator=(const treeture&) = delete;

		treeture& operator=(treeture&&) {
			return *this;
		}


		// --- treeture Operators ---

		T get() {
			this->wait();
			return this->task.getResult();
		}

	};

	template<typename A, typename B>
	class treeture<void,A,B> : public detail::treeture_base<void,A,B> {

	public:

		// --- Constructors ---

		treeture() {}

		treeture(const treeture&) = delete;

		treeture(treeture&&) {}


		// --- Operators ---

		treeture& operator=(const treeture&) = delete;

		treeture& operator=(treeture&&) {
			return *this;
		}


		// --- treeture Operators ---

		void get() {
			this->wait();
		}

	};


	// --- template utilities ---

	template<typename T>
	struct remove_treeture {
		using type = T;
	};

	template<typename T>
	struct remove_treeture<treeture<T>> {
		using type = T;
	};

	template<typename T>
	using remove_treeture_t = typename remove_treeture<T>::type;


	// --- factories ---

	static treeture<void> done() { return treeture<void>(); }

	template<typename T>
	treeture<T> done(const T& t) {
		return treeture<T>();
	}

	template<typename T>
	treeture<T> done(T&& t) {
		return treeture<T>();
	}


	template<
		typename Progress,
		typename Split,
		typename R = typename std::enable_if_t<
				std::is_same<
					std::result_of_t<Progress()>,
					remove_treeture_t<std::result_of_t<Split()>>
				>::value,
				std::result_of_t<Progress()>
			>
	>
	treeture<R> spawn(const Progress& progress, const Split& split) {
		return treeture<R>();
	}

	template<
		typename Action,
		typename R = std::result_of_t<Action>
	>
	treeture<R> spawn(const Action& action) {
		return spawn(action,action);
	}

	template<typename A, typename B, typename C, typename R = std::result_of_t<C(A,B)>>
	static treeture<R,A,B> parallel(treeture<A>&& a, treeture<B>&& b, const C& c) { return treeture<void>(); }

	template<typename A, typename B, typename C, typename R = std::result_of_t<C(A,B)>>
	static treeture<R> sequential(treeture<A>&& a, treeture<B>&& b, const C& c) { return treeture<void>(); }



	// --- more utilities ---

	treeture<void> parallel() {
		return done();
	}

	template<typename R, typename A, typename B>
	treeture<void> parallel(treeture<R,A,B>&& single) {
		return std::move(single);
	}

	template<typename R, typename A, typename B, typename ... Rest>
	treeture<void> parallel(treeture<R,A,B>&& first, Rest&& ... rest) {
		return parallel(first,parallel(rest...), [](auto,auto){});
	}

	treeture<void> sequential() {
		return done();
	}

	template<typename A>
	treeture<void> sequential(treeture<A>&& single) {
		return std::move(single);
	}

	template<typename A, typename ... Rest>
	treeture<void> sequential(treeture<A>&& first, Rest&& ... rest) {
		return sequential(first,sequential(rest...), [](auto,auto){});
	}
*/

} // end namespace core
} // end namespace api
} // end namespace allscale

