#pragma once

#include <bitset>
#include <memory>
#include <type_traits>

#ifdef __cilk
    #include <cilk/cilk.h>
#else
	#define cilk_spawn
	#define cilk_sync
#endif

namespace allscale {
namespace api {
namespace core {

	// ---------------------------------------------------------------------------------------------
	//											  Tasks
	// ---------------------------------------------------------------------------------------------


	class Task;
	using TaskPtr = std::shared_ptr<Task>;


	// the RT's interface to a task
	class Task {

		template<typename Process, typename Split>
		friend class SplitableTask;

		bool done;

	protected:

		TaskPtr left;
		TaskPtr right;

		bool parallel;

		Task(bool done) : done(done), parallel(false) {};

		Task(TaskPtr&& left, TaskPtr&& right, bool parallel)
			: done(false), left(left), right(right), parallel(parallel) {}

	public:

		virtual ~Task(){};

		void process() {
			if (done) return;
			compute();
			done = true;
		}

		virtual void compute() {

			assert(left || right);

			// if there is only one child => run this one
			if (!right) {
				left->process();
				return;
			}

			// if there are more => run both -- TODO: if necessary, in parallel
			if (parallel) {
				cilk_spawn left->process();
				right->process();
				cilk_sync;
			} else {
				left->process();
				right->process();
			}
		}

		virtual void split() {}

		TaskPtr getLeft() const {
			return left;
		}

		TaskPtr getRight() const {
			return right;
		}

		bool runsSubTasksParallel() const {
			return parallel;
		}

	};


	template<typename Process>
	class SimpleTask : public Task {

		Process task;

	public:

		SimpleTask() : Task(true) {}

		SimpleTask(Process c)
			: Task(false), task(c) {}


		void compute() override {
			task();
		}

	};


	template<typename Process, typename Split>
	class SplitableTask : public Task {

		Process task;
		Split decompose;

	public:

		SplitableTask() : Task(true) {}

		SplitableTask(Process c, Split d)
			: Task(false), task(c), decompose(d) {}

		void compute() override {

			// if not split
			if (!left) {
				task();
				return;
			}

			// otherwise, run default
			Task::compute();
		}


		void split() override;

	};

	class SplitTask : public Task {

	public:

		SplitTask(TaskPtr&& left, TaskPtr&& right, bool parallel)
			: Task(std::move(left),std::move(right), parallel) {}

		void split() override {
			assert(false);		// should not be reached
		}

	};


	// ---------------------------------------------------------------------------------------------
	//											Treetures
	// ---------------------------------------------------------------------------------------------



	class BitQueue {

		uint64_t buffer;
		std::size_t size;

	public:

		BitQueue() : buffer(0), size(0) {};

		bool empty() const {
			return size == 0;
		}

		void put(bool bit) {
			assert(size < 64);
			buffer = (buffer << 1) | (bit ? 1 : 0);
			size++;
		}

		bool get() {
			return buffer & (1<<(size-1));
		}

		bool pop() {
			assert(!empty());
			size--;
			return buffer & (1 << size);
		}

	};


	class treeture {

		TaskPtr task;

		BitQueue queue;

	public:

		treeture() {}
		treeture(const treeture&) = default;
		treeture(treeture&& other) = default;

		treeture& operator=(const treeture&) = default;
		treeture& operator=(treeture&& other) = default;

		void wait() {
			narrow();	// see whether we can further descent
			if (task) task->process();
		}

		treeture& descentLeft() {
			queue.put(0);
			return *this;
		}

		treeture& descentRight() {
			queue.put(1);
			return *this;
		}

		treeture getLeft() const {
			if (task) return *this;
			return treeture(*this).descentLeft();
		}

		treeture getRight() const {
			if (task) return *this;
			return treeture(*this).descentRight();
		}

	private:

		template<typename Process, typename Split>
		friend class SplitableTask;

		template<typename Action>
		friend treeture spawn(const Action&);

		template<typename Process, typename Split>
		friend treeture spawn(const Process&, const Split&);

		template<typename A, typename B>
		friend treeture sequence(A&&, B&&);

		template<typename A, typename B>
		friend treeture parallel(A&&, B&&);

		treeture(TaskPtr&& task) : task(task) {}

		void narrow() {
			if (!task) return;
			while(!queue.empty()) {
				TaskPtr next = (queue.get()) ? task->getLeft() : task->getRight();
				if (!next) return;
				queue.pop();
				task = next;
			}
		}

	};

	template<typename Process, typename Split>
	void SplitableTask<Process,Split>::split() {
		// split this task
		const treeture& sub = decompose();
		assert(sub.queue.empty());

		// check whether the split just produced a single task
		if (!sub.task->left) {
			// this becomes the only child
			this->left = std::move(sub.task);
		} else {
			// register left and right
			this->left = std::move(sub.task->left);
			this->right = std::move(sub.task->right);

			// copy parallel flag
			this->parallel = sub.task->runsSubTasksParallel();
		}
	}


	// ---------------------------------------------------------------------------------------------
	//											Operators
	// ---------------------------------------------------------------------------------------------



	inline treeture done() {
		return treeture();
	}

	template<typename Action>
	treeture spawn(const Action& a) {
		return TaskPtr(std::make_shared<SimpleTask<Action>>(a));
	}

	template<typename Process, typename Split>
	treeture spawn(const Process& a, const Split& b) {
		return TaskPtr(std::make_shared<SplitableTask<Process,Split>>(a,b));
	}

	inline treeture to_treeture(treeture&& a) {
		return std::move(a);
	}

	template<typename A>
	inline treeture to_treeture(A&& a) {
		return spawn(a);
	}

	inline treeture parallel() {
		return done();
	}

	template<typename A>
	inline treeture parallel(A&& a) {
		return to_treeture(a);
	}

	template<typename A, typename B>
	inline treeture parallel(A&& a, B&& b) {
		return treeture(std::make_shared<SplitTask>(std::move(to_treeture(std::move(a)).task),std::move(to_treeture(std::move(b)).task),true));
	}

	template<typename A, typename ... Rest>
	inline treeture parallel(A&& first, Rest&& ... rest) {
		return parallel(to_treeture(std::move(first)), parallel(rest...));
	}

	inline treeture sequence() {
		return done();
	}

	template<typename A>
	inline treeture sequence(A&& a) {
		return a;
	}

	template<typename A, typename B>
	inline treeture sequence(A&& a, B&& b) {
		return treeture(std::make_shared<SplitTask>(std::move(to_treeture(std::move(a)).task),std::move(to_treeture(std::move(b)).task),false));
	}

	template<typename A, typename ... Rest>
	inline treeture sequence(A&& first, Rest&& ... rest) {
		return sequence(to_treeture(std::move(first)), sequence(rest...));
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

