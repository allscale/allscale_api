#pragma once

#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <memory>
#include <type_traits>


#ifdef __cilk
    #include <cilk/cilk.h>
    #define PARALLEL(A,B) { \
		cilk_spawn (A);     \
		B;                  \
		cilk_sync;          \
	}
#elif _OPENMP
	#define PARALLEL(A,B) _Pragma ("omp parallel sections") {  \
		A;                                                     \
		_Pragma ("omp section")                                \
		B;
	}
#else
    #define PARALLEL(A,B) { A; B; }
#endif

namespace allscale {
namespace api {
namespace core {
namespace impl {
#ifdef OMP_CILK_IMPL
inline
#endif
namespace omp_cilk {

	inline const std::string& getImplementationName() {
		static const std::string name = "OpenMP/Cilk";
		return name;
	}


	// ---------------------------------------------------------------------------------------------
	//											  Tasks
	// ---------------------------------------------------------------------------------------------


	class TaskBase;

	template<typename T>
	class Task;


	using TaskBasePtr = std::shared_ptr<TaskBase>;

	template<typename T>
	using TaskPtr = std::shared_ptr<Task<T>>;

	// the RT's interface to a task
	class TaskBase {

		std::atomic<bool> done;

		TaskBasePtr left;
		TaskBasePtr right;

	public:

		TaskBase(bool done) : done(done) {}
		TaskBase(TaskBasePtr&& left, TaskBasePtr&& right)
			: done(false), left(std::move(left)), right(std::move(right)) {}

		virtual ~TaskBase() {}

		virtual void process() {
			if (done) return;
			compute();
			done = true;
			left.reset();
			right.reset();
		}

		virtual void split() {}

		bool isDone() const {
			return done;
		}

		void wait() {
			if (done) return;
			split();		// split everything if possible
			process();		// process result
		}

		TaskBasePtr getLeft() const {
			return left;
		}

		TaskBasePtr getRight() const {
			return right;
		}

	protected:

		virtual void compute() =0;

		void setLeft(const TaskBasePtr& left) {
			this->left = left;
		}

		void setRight(const TaskBasePtr& right) {
			this->right = right;
		}

		void processSubTasks(bool parallel) {
			if (!parallel) {
				// run sequentially
				if (left) left->wait();
				if (right) right->wait();
				return;
			}

			// run in parallel
			PARALLEL(
					[&](){ if (left) left->wait(); }(),
					[&](){ if (right) right->wait(); }()
			);
		}

	};


	// a task computing a value of type T
	template<typename T>
	class Task : public TaskBase {

		T value;

	public:

		Task() : TaskBase(false) {}

		Task(const T& value)
			: TaskBase(true), value(value) {}

		Task(TaskBasePtr&& left, TaskBasePtr&& right)
			: TaskBase(std::move(left),std::move(right)) {}


		virtual ~Task(){};

		const T& getValue() const {
			return value;
		}

	protected:

		void compute() override {
			value = computeValue();
		}

		virtual T computeValue() {
			return value;
		};

	};

	template<>
	class Task<void> : public TaskBase {
	public:

		Task() : TaskBase(false) {}

		Task(TaskBasePtr&& left, TaskBasePtr&& right)
			: TaskBase(std::move(left),std::move(right)) {}

		virtual ~Task(){};

		void getValue() const {
			// nothing
		}

	protected:

		void compute() override {
			computeValue();
		}

		virtual void computeValue() {
		};

	};


	template<
		typename Process,
		typename R = std::result_of_t<Process()>
	>
	class SimpleTask : public Task<R> {

		Process task;

	public:

		SimpleTask(const Process& task) : task(task) {}

		R computeValue() override {
			return task();
		}

	};


	template<typename R, typename A, typename B, typename C>
	class SplitTask : public Task<R> {

		Task<A>& left;
		Task<B>& right;

		C merge;

		bool parallel;

	public:

		SplitTask(TaskPtr<A>&& left, TaskPtr<B>&& right, C&& merge, bool parallel)
			: Task<R>(std::move(left),std::move(right)),
			  left(static_cast<Task<A>&>(*this->getLeft())),
			  right(static_cast<Task<B>&>(*this->getRight())),
			  merge(merge),
			  parallel(parallel) {}

		R computeValue() override {
			this->processSubTasks(parallel);
			return merge(left.getValue(),right.getValue());
		}

		void split() override {
			// ignore
		}

	};

	template<typename A, typename B>
	class SplitTask<void,A,B,void> : public Task<void> {

		bool parallel;

	public:

		SplitTask(TaskBasePtr&& left, TaskBasePtr&& right, bool parallel)
			: Task<void>(std::move(left),std::move(right)), parallel(parallel) {}

		void computeValue() override {
			this->processSubTasks(parallel);
		}

		void split() override {
			// ignore
		}

	};

	template<typename A, typename B, typename C, typename R = std::result_of_t<C(A,B)>>
	TaskPtr<R> make_split_task(TaskPtr<A>&& left, TaskPtr<B>&& right, C&& merge, bool parallel) {
		return std::make_shared<SplitTask<R,A,B,C>>(std::move(left), std::move(right), std::move(merge), parallel);
	}

	inline TaskPtr<void> make_split_task(TaskBasePtr&& left, TaskBasePtr&& right, bool parallel) {
		return std::make_shared<SplitTask<void,void,void,void>>(std::move(left), std::move(right), parallel);
	}



	// ---------------------------------------------------------------------------------------------
	//											Treetures
	// ---------------------------------------------------------------------------------------------



	template<typename T>
	class treeture;


	template<>
	class treeture<void> {

		template<typename Process, typename Split, typename R>
		friend class SplitableTask;

		TaskBasePtr task;

	protected:

		treeture() {}

		treeture(const TaskBasePtr& t) : task(t) {}
		treeture(TaskBasePtr&& task) : task(std::move(task)) {}

	public:

		using value_type = void;

		treeture(const treeture&) = default;
		treeture(treeture&& other) = default;

		treeture& operator=(const treeture&) = default;
		treeture& operator=(treeture&& other) = default;

		// support implicit cast to void treeture
		template<typename T>
		treeture(const treeture<T>& t) : task(t.task) {}

		// support implicit cast to void treeture
		template<typename T>
		treeture(treeture<T>&& t) : task(std::move(t.task)) {}

		void start() {
			// nothing to do here ..
		}

		void wait() const {
			if(task) task->wait();
		}

		void get() {
			wait();
		}

		treeture& descentLeft() {
			return *this;		// not supported => overapproximate
		}

		treeture& descentRight() {
			return *this;		// not supported => overapproximate
		}

		treeture getLeft() const {
			return treeture(*this).descentLeft();
		}

		treeture getRight() const {
			return treeture(*this).descentRight();
		}


		// -- factories --

		static treeture done() {
			return treeture();
		}

		template<typename Action>
		static treeture spawn(const Action& a) {
			return treeture(TaskBasePtr(std::make_shared<SimpleTask<Action>>(a)));
		}

		template<typename Process, typename Split>
		static treeture spawn(const Process&, const Split& s) {
			return s();
		}

		template<typename A, typename B>
		static treeture combine(treeture<A>&& a, treeture<B>&& b, bool parallel) {
			return treeture(TaskBasePtr(make_split_task(std::move(a.task),std::move(b.task),parallel)));
		}

	};

	template<typename T>
	class treeture {

		friend class treeture<void>;

		TaskPtr<T> task;

	protected:

		treeture() {}

		treeture(const T& value) : task(std::make_shared<Task<T>>(value)) {}

		treeture(TaskPtr<T>&& task) : task(std::move(task)) {}

	public:

		using value_type = T;

		treeture(const treeture&) = delete;
		treeture(treeture&& other) : task(std::move(other.task)) {}

		treeture& operator=(const treeture&) = delete;
		treeture& operator=(treeture&& other) {
			task = std::move(other.task);
			return *this;
		}

		void start() {
			// nothing to do here ..
		}

		void wait() const {
			task->wait();
		}

		const T& get() {
			wait();
			return task->getValue();
		}

		treeture<void> getLeft() const {
			return treeture<void>(*this).getLeft();
		}

		treeture<void> getRight() const {
			return treeture<void>(*this).getRight();
		}

		// -- factories --

		static treeture done(const T& value) {
			return treeture(value);
		}

		template<typename Action>
		static treeture spawn(const Action& a) {
			return treeture(TaskPtr<T>(std::make_shared<SimpleTask<Action>>(a)));
		}

		template<typename Process, typename Split>
		static treeture spawn(const Process&, const Split& s) {
			return s();
		}

		template<typename A, typename B, typename C>
		static treeture combine(treeture<A>&& a, treeture<B>&& b, C&& merge, bool parallel = true) {
			return treeture(make_split_task(std::move(a.task),std::move(b.task),std::move(merge),parallel));
		}

	};


} // end namespace omp_cilk
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale

