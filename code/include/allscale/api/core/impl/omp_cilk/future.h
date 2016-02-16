#pragma once

#include <functional>
#include <type_traits>
#include <memory>
#include <vector>

#ifdef __cilk
    #include <cilk/cilk.h>
    #define parallel_for cilk_for
#elif _OPENMP
    #define parallel_for _Pragma ("omp parallel for") for
#else
    #define parallel_for for
#endif

#define PAREC_IMPL "OpenMP/Cilk"

namespace allscale {
namespace api {
namespace core {
inline namespace omp_cilk {

	template<typename Result>
	class Future;

	namespace internal {

		// the kinds of composed futures
		enum class Kind {
			Atomic,
			Sequential,
			Parallel
		};

		template<typename Result>
		class TaskReference;

		template<typename Task, typename Result>
		class TaskBase {

		public:

			using work_t = std::function<Result()>;
			using aggregator_t = Result(*)(const std::vector<Future<Result>>&);

		private:

			// the kind of composed future represented
			Kind kind;

			// a flag whether it is complete (if composed, a cache)
			mutable bool done;

			// for atomic tasks: the associated task
			work_t work;

			// for composed tasks: the list of sub-tasks
			std::vector<TaskReference<Result>> subTasks;

		protected:

			TaskBase(const work_t& work)
				: kind(Kind::Atomic), done(false), work(work) {}

			template<typename ... Subs>
			TaskBase(Kind kind, Subs&& ... subs)
				: kind(kind), done(false), subTasks(sizeof...(subs)) {
				fill(0,std::move(subs)...);
			}

			void fill(unsigned) {}

			template<typename First, typename ... Subs>
			void fill(unsigned i, First&& first, Subs&&  ... rest) {
				subTasks[i] = std::move(first);
				fill(i+1,std::move(rest)...);
			}

		public:

			TaskBase() : kind(Kind::Atomic), done(true) {}

			TaskBase(const TaskBase&) = delete;
			TaskBase(TaskBase&&) = delete;

			TaskBase& operator=(const TaskBase& other) = delete;
			TaskBase& operator=(TaskBase&& other) = delete;

			bool isDone() const {
				if (done) return true;
				if (isAtom()) return false;
				for(const auto& cur : subTasks) {
					if (!cur.isDone()) return false;
				}

				// notify implementation on completion
				static_cast<const Task&>(*this).completed();

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

			const std::vector<TaskReference<Result>>& getSubTasks() const {
				return subTasks;
			}

			void wait() const {
				if (done) return;
				if (kind == Kind::Atomic) {
					if(!done) {
						static_cast<const Task&>(*this).process();
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
				static_cast<const Task&>(*this).completed();

				done = true;
			}

		protected:

			const work_t& getWork() const {
				return work;
			}

		};

		template<typename Result>
		class Task : public TaskBase<Task<Result>,Result> {

			using super = TaskBase<Task,Result>;

		public:

			using work_t = typename super::work_t;
			using aggregator_t = typename super::aggregator_t;

		private:

			friend TaskBase<Task,Result>;

			mutable Result value;

			aggregator_t aggregator;

		public:

			Task(const Result& result)
				: super(), value(result), aggregator(nullptr) {}

			Task(const work_t& work)
				: super(work), aggregator(nullptr) {}

			template<typename ... Subs>
			Task(Kind kind, const aggregator_t& aggregator, Subs&& ... subs)
				: super(kind, std::move(subs)...), aggregator(aggregator) {}

			~Task() {
				aggregator = nullptr;
			}

			const Result& getValue() const {
				return value;
			}

		private:

			void process() const {
				value = this->getWork()();
			}

			void completed() const {
				if (aggregator) {
					value = aggregator(reinterpret_cast<const std::vector<Future<Result>>&>(this->getSubTasks()));
				}
			}

		};

		template<>
		class Task<void> : public TaskBase<Task<void>,void> {

			friend TaskBase<Task,void>;

		public:

			using super = TaskBase<Task,void>;
			using work_t = std::function<void()>;

			Task(const work_t& work)
				: super(work) {}

			template<typename ... Subs>
			Task(Kind kind, Subs&& ... subs)
				: super(kind, std::move(subs)...) {}

		private:

			void process() const {
				this->getWork()();
			}

			void completed() const {
				// nothing to do
			}
		};

		template<typename Result>
		class TaskReference {

		protected:

			using work_t = typename Task<Result>::work_t;
			using aggregator_t = typename Task<Result>::aggregator_t;

		private:

			std::shared_ptr<Task<Result>> task;

		protected:

			TaskReference(const Result& result)
				: task(std::make_shared<Task<Result>>(result)) {}

			TaskReference(const work_t& work)
				: task(std::make_shared<Task<Result>>(work)) {}

			template<typename ... Subs>
			TaskReference(Kind kind, Subs&& ... subs)
				: task(std::make_shared<Task<Result>>(kind, std::move(subs)...)) {}

			template<typename ... Subs>
			TaskReference(Kind kind, const aggregator_t& aggregator, Subs&& ... subs)
				: task(std::make_shared<Task<Result>>(kind, aggregator, std::move(subs)...)) {}

		public:

			TaskReference() : task() {}

			TaskReference(const TaskReference& other) {
				task = other.task;
			}

			TaskReference(TaskReference&& other) {
				task.swap(other.task);
			}

			TaskReference& operator=(const TaskReference& other) {
				task = other.task;
				return *this;
			}

			TaskReference& operator=(TaskReference&& other) {
				task.swap(other.task);
				return *this;
			}

			bool valid() const {
				return (bool)task;
			}

			bool isDone() const {
				return !task || task->isDone();
			}

			bool isAtom() const {
				return !task || task->isAtom();
			}

			bool isSequence() const {
				return task && task->isSequence();
			}

			bool isParallel() const {
				return task && task->isParallel();
			}

			bool isComposed() const {
				return !isAtom();
			}

			const std::vector<TaskReference>& getSubTasks() const {
				static const std::vector<TaskReference> empty;
				return (task) ? task->getSubTasks() : empty;
			}

			void wait() const {
				if (!task) return;
				task->wait();
			}

			const Result& get() const {
				static const Result default_value = Result();
				wait();
				return (task) ? task->getValue() : default_value;
			}

		};

		template<>
		class TaskReference<void> {

		protected:

			using work_t = typename Task<void>::work_t;
			using aggregator_t = typename Task<void>::aggregator_t;

		private:

			std::shared_ptr<Task<void>> task;

		protected:

			TaskReference(const work_t& work)
				: task(std::make_shared<Task<void>>(work)) {}

			template<typename ... Subs>
			TaskReference(Kind kind, Subs&& ... subs)
				: task(std::make_shared<Task<void>>(kind, std::move(subs)...)) {}

		public:

			TaskReference() : task() {}

			TaskReference(const TaskReference& other) {
				task = other.task;
			}

			TaskReference(TaskReference&& other) {
				task.swap(other.task);
			}

			TaskReference& operator=(const TaskReference& other) {
				task = other.task;
				return *this;
			}

			TaskReference& operator=(TaskReference&& other) {
				task.swap(other.task);
				return *this;
			}

			bool valid() const {
				return (bool)task;
			}

			bool isDone() const {
				return !task || task->isDone();
			}

			bool isAtom() const {
				return !task || task->isAtom();
			}

			bool isSequence() const {
				return task && task->isSequence();
			}

			bool isParallel() const {
				return task && task->isParallel();
			}

			bool isComposed() const {
				return !isAtom();
			}

			const std::vector<TaskReference>& getSubTasks() const {
				static const std::vector<TaskReference> empty;
				return (task) ? task->getSubTasks() : empty;
			}

			void wait() const {
				if (!task) return;
				task->wait();
			}

			void get() const {
				wait();
			}

		};


		template<typename Result>
		class FutureBase : public TaskReference<Result> {

			using super = TaskReference<Result>;

		public:

			using task_reference = TaskReference<Result>;

		protected:

			using work_t = typename super::work_t;
			using aggregator_t = typename super::aggregator_t;

			FutureBase(const Result& result)
				: super(result) {}

			FutureBase(const work_t& work)
				: super(work) {}

			template<typename ... Subs>
			FutureBase(Kind kind, const aggregator_t& aggregator, Subs&& ... subs)
				: super(kind,aggregator,std::move(subs)...) {}

		public:

			FutureBase() : super() {}

			FutureBase(const FutureBase&) = delete;
			FutureBase(FutureBase&& other) = default;

			~FutureBase() {
				this->wait();
			}

			FutureBase& operator=(const FutureBase& other) = delete;
			FutureBase& operator=(FutureBase&& other) = default;

			const task_reference& getTaskReference() const {
				return *this;
			}

		};

		template<>
		class FutureBase<void> : public TaskReference<void> {

			using super = TaskReference<void>;

		protected:

			using work_t = typename super::work_t;
			using aggregator_t = typename super::aggregator_t;

			FutureBase(const work_t& work)
				: super(work) {}

			template<typename ... Subs>
			FutureBase(Kind kind, Subs&& ... subs)
				: super(kind,std::move(subs)...) {}

		public:

			FutureBase() : super() {}

			FutureBase(const FutureBase&) = delete;
			FutureBase(FutureBase&& other) = default;

			~FutureBase() {
				this->wait();
			}

			FutureBase& operator=(const FutureBase& other) = delete;
			FutureBase& operator=(FutureBase&& other) = default;

		};

	}

	// ---------------------------------------------------------------------------------------------
	//											Futures
	// ---------------------------------------------------------------------------------------------

	/**
	 * The general future implementation.
	 */
	template<typename T>
	class Future : public internal::FutureBase<T> {

		using super = internal::FutureBase<T>;
		using work_t = typename super::work_t;
		using aggregator_t = typename super::aggregator_t;

	private:

		Future(const work_t& work)
			: super(work) {}

		template<typename ... Subs>
		Future(internal::Kind kind, const aggregator_t& aggregator, Subs&& ... subs)
			: super(kind, aggregator, std::move(subs)...) {}

	public:

		Future(const T& value = T())
			: super(value) {}

		Future(const Future&) = delete;

		Future(Future&& other)
			: super(std::move(other)) {}

		Future& operator=(const Future& other) = delete;

		Future& operator=(Future&& other) {
			return *new (this) Future(std::move(other));
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
	class Future<void> : public internal::FutureBase<void> {

		using super = internal::FutureBase<void>;
		using work_t = typename super::work_t;

		Future(const work_t& work)
			: super(work) {}

		template<typename ... Subs>
		Future(internal::Kind kind, Subs&& ... subs)
			: super(kind, std::move(subs)...) {}

	public:

		Future() : super() {}

		Future(const Future&) = delete;

		Future(Future&& other)
			: super(std::move(other)) {}

		Future& operator=(const Future& other) = delete;

		Future& operator=(Future&& other) {
			return *new (this) Future(std::move(other));
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

	template<
		typename T,
		typename R = typename std::result_of<T()>::type
	>
	Future<R> spawn(const T& task) {
		return atom(task);
	}

	template<
		typename E,
		typename S,
		typename RE = typename std::result_of<E()>::type,
		typename RS = typename std::result_of<S()>::type
	>
	typename std::enable_if<
		std::is_same<Future<RE>,RS>::value,
		Future<RE>
	>::type
	spawn(const E&, const S& split) {
		// this implementation always splits directly
		return split();
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


