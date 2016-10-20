#pragma once

#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <type_traits>

#include <pthread.h>

#include "allscale/api/core/impl/reference/lock.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
#ifdef REFERENCE_IMPL
inline
#endif
namespace reference {

	inline const std::string& getImplementationName() {
		static const std::string name = "Reference SharedMemory";
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

		void wait();

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

		void processSubTasks(bool parallel);

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


	template<
		typename Process,
		typename Split,
		typename R = std::result_of_t<Process()>
	>
	class SplitableTask : public Task<R> {

		Process task;
		Split decompose;

		TaskPtr<R> subTask;

	public:

		SplitableTask(const Process& c, const Split& d)
			: task(c), decompose(d) {}

		R computeValue() override {
			// if split
			if (subTask) {
				// compute sub-task
				subTask->process();
				return subTask->getValue();
			}
			return task();
		}

		void split() override;

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

	template<typename T>
	class treeture;


	template<>
	class treeture<void> {

		template<typename Process, typename Split, typename R>
		friend class SplitableTask;

		TaskBasePtr task;

		BitQueue queue;

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

		void wait();

		void get() {
			wait();
		}

		treeture& descentLeft() {
			if (!task) return *this;
			queue.put(0);
			return *this;
		}

		treeture& descentRight() {
			if (!task) return *this;
			queue.put(1);
			return *this;
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
		static treeture spawn(const Process& p, const Split& s) {
			return treeture(TaskBasePtr(std::make_shared<SplitableTask<Process,Split>>(p,s)));
		}

		template<typename A, typename B>
		static treeture combine(treeture<A>&& a, treeture<B>&& b, bool parallel) {
			return treeture(TaskBasePtr(make_split_task(std::move(a.task),std::move(b.task),parallel)));
		}

	private:

		void narrow() {
			if (!task) return;
			while(!queue.empty()) {
				TaskBasePtr next = (queue.get()) ? task->getLeft() : task->getRight();
				if (!next) return;
				queue.pop();
				task = next;
			}
		}

	};

	template<typename T>
	class treeture {

		friend class treeture<void>;

		template<typename Process, typename Split, typename R>
		friend class SplitableTask;

		TaskPtr<T> task;

	protected:

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

		void wait();

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
		static treeture spawn(const Process& p, const Split& s) {
			return treeture(TaskPtr<T>(std::make_shared<SplitableTask<Process,Split>>(p,s)));
		}

		template<typename A, typename B, typename C>
		static treeture combine(treeture<A>&& a, treeture<B>&& b, C&& merge, bool parallel = true) {
			return treeture(make_split_task(std::move(a.task),std::move(b.task),std::move(merge),parallel));
		}

	};

	template<typename Process, typename Split, typename R>
	void SplitableTask<Process,Split,R>::split() {
		// decompose this task
		subTask = std::static_pointer_cast<Task<R>>(decompose().task);

		// mutate this task into a split task
		this->setLeft(subTask->getLeft());
		this->setRight(subTask->getRight());
	}


	// ---------------------------------------------------------------------------------------------
	//											Runtime
	// ---------------------------------------------------------------------------------------------

	namespace runtime {

		// -- Declarations --

		const bool DEBUG = false;

		std::mutex g_log_mutex;

		#define LOG(MSG) \
			{  \
				if (DEBUG) { \
					std::thread::id this_id = std::this_thread::get_id(); \
					std::lock_guard<std::mutex> lock(g_log_mutex); \
					std::cout << "Thread " << this_id << ": " << MSG << "\n"; \
				} \
			}

		const bool DEBUG_SCHEDULE = false;

		#define LOG_SCHEDULE(MSG) \
			{  \
				if (DEBUG_SCHEDULE) { \
					std::thread::id this_id = std::this_thread::get_id(); \
					std::lock_guard<std::mutex> lock(g_log_mutex); \
					std::cout << "Thread " << this_id << ": " << MSG << "\n"; \
				} \
			}


		// -----------------------------------------------------------------
		//						    Worker Pool
		// -----------------------------------------------------------------

		struct Worker;

		thread_local static Worker* tl_worker = nullptr;

		static void setCurrentWorker(Worker& worker) {
			tl_worker = &worker;
		}

		static Worker& getCurrentWorker();


		template<typename T, size_t Capacity>
		class SimpleQueue {

		public:

			static const size_t capacity = Capacity;

		private:

			static const size_t buffer_size = capacity + 1;

			mutable SpinLock lock;

			std::array<T,buffer_size> data;

			size_t front;
			size_t back;

		public:

			SimpleQueue() : lock(), front(0), back(0) {
				for(auto& cur : data) cur = T();
			}

			bool empty() const {
				return front == back;
			}
			bool full() const {
				return ((back + 1) % buffer_size) == front;
			}

			bool push_front(const T& t) {
				lock.lock();
				if (full()) {
					lock.unlock();
					return false;
				}
				front = (front - 1 + buffer_size) % buffer_size;
				data[front] = t;
				lock.unlock();
				return true;
			}

			bool push_back(const T& t) {
				lock.lock();
				if (full()) {
					lock.unlock();
					return false;
				}
				data[back] = t;
				back = (back + 1) % buffer_size;
				lock.unlock();
				return true;
			}

			T pop_front() {
				lock.lock();
				if (empty()) {
					lock.unlock();
					return T();
				}
				T res(std::move(data[front]));
				front = (front + 1) % buffer_size;
				lock.unlock();
				return res;
			}

			T pop_back() {
				lock.lock();
				if (empty()) {
					lock.unlock();
					return T();
				}
				back = (back - 1 + buffer_size) % buffer_size;
				T res(std::move(data[back]));
				lock.unlock();
				return res;
			}

			size_t size() const {
				lock.lock();
				size_t res = (back >= front) ? (back - front) : (buffer_size - (front - back));
				lock.unlock();
				return res;
			}

			friend std::ostream& operator<<(std::ostream& out, const SimpleQueue& queue) {
				return out << "[" << queue.data << "," << queue.front << " - " << queue.back << "]";
			}

		};

		namespace detail {

			/**
			 * A utility to fix the affinity of the current thread to the given core.
			 */
			void fixAffinity(int core) {
				static const int num_cores = std::thread::hardware_concurrency();
				cpu_set_t mask;
				CPU_ZERO(&mask);
				CPU_SET(core % num_cores, &mask);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask);
			}

		}

		class WorkerPool;

		struct Worker {

			WorkerPool& pool;

			volatile bool alive;

			SimpleQueue<TaskBasePtr,8> queue;

			std::thread thread;

			unsigned id;

			unsigned random_seed;

		public:

			Worker(WorkerPool& pool, unsigned id)
				: pool(pool), alive(true), id(id), random_seed(id) { }

			Worker(const Worker&) = delete;
			Worker(Worker&&) = delete;

			Worker& operator=(const Worker&) = delete;
			Worker& operator=(Worker&&) = delete;

			void start() {
				thread = std::thread([&](){ run(); });
			}

			void poison() {
				alive = false;
			}

			void join() {
				thread.join();
			}

		private:

			void run();

		public:

			void schedule(const TaskBasePtr& task);

			bool schedule_step();

		};




		class WorkerPool {

			std::vector<Worker*> workers;

			// tools for managing idle threads
			std::mutex m;
			std::condition_variable cv;

			WorkerPool() {

				int numWorkers = std::thread::hardware_concurrency();

				// parse environment variable
				if (char* val = std::getenv("NUM_WORKERS")) {
					auto userDef = std::atoi(val);
					if (userDef != 0) numWorkers = userDef;
				}

				// reduce by one, since main thread also counts
				numWorkers--;

				// must be at least one worker
				if (numWorkers < 1) numWorkers = 1;

				// create workers
				for(int i=0; i<numWorkers; ++i) {
					workers.push_back(new Worker(*this,i+1));
				}

				// start workers
				for(auto& cur : workers) cur->start();

				// fix affinity of main thread
				detail::fixAffinity(0);
			}

			~WorkerPool() {
				// shutdown threads

				// poison all workers
				for(auto& cur : workers) {
					cur->poison();
				}

				// make work available
				workAvailable();

				// wait for their death
				for(auto& cur : workers) {
					cur->join();
				}

				// free resources
				for(auto& cur : workers) {
					delete cur;
				}

			}

		public:

			static WorkerPool& getInstance() {
				static WorkerPool pool;
				return pool;
			}

			int getNumWorkers() const {
				return workers.size();
			}

			Worker& getWorker(int i) {
				return *workers[i];
			}

			Worker& getWorker() {
				return getWorker(0);
			}

		protected:

			friend Worker;

			void waitForWork() {
				std::unique_lock<std::mutex> lk(m);
				cv.wait(lk);
			}

			void workAvailable() {
				// wake up all workers
				cv.notify_all();
			}

		};

		static Worker& getCurrentWorker() {
			if (tl_worker) return *tl_worker;
			return WorkerPool::getInstance().getWorker();
		}

		inline void Worker::run() {

			// fix affinity
			detail::fixAffinity(id);

			// register worker
			setCurrentWorker(*this);

			// start processing loop
			while(alive) {

				// count number of idle cylces
				int idle_cycles = 0;

				// conduct a schedule step
				while(alive && !schedule_step()) {
					// increment idle counter
					++idle_cycles;

					// wait a moment
					cpu_relax();

					// if there was no work for quite some time
					if(idle_cycles > 100000) {
						// wait for work
						pool.waitForWork();
				
						// reset cycles counter
						idle_cycles = 0;
					}
				}
			}

			// done

		}

		void Worker::schedule(const TaskBasePtr& task) {

			// add task to queue
			LOG_SCHEDULE( "Queue size before: " << queue.size() << "/" << queue.capacity );

			// enqueue task into work queue
			if (queue.push_back(task)) {
				// signal available work
				if (queue.size() > queue.capacity/2) {
					pool.workAvailable();
				}

				// that's it
				return;
			}

			// log new queue length
			LOG_SCHEDULE( "Queue size after: " << queue.size() << "/" << queue.capacity );


			// since queue is full, process directly
			task->process();
		}


		inline bool Worker::schedule_step() {

			// process a task from the local queue
			if (TaskBasePtr t = queue.pop_front()) {

				// if the queue is not full => create more tasks
				if (queue.size() < (queue.capacity*3)/4) {

					LOG_SCHEDULE( "Splitting tasks @ queue size: " << queue.size() << "/" << queue.capacity );

					// split task
					t->split();
				}

				// process this task
				t->process();
				return true;
			}

			// check that there are other workers
			int numWorker = pool.getNumWorkers();
			if (numWorker <= 1) return false;

			// otherwise, steal a task from another worker
			Worker& other = pool.getWorker(rand_r(&random_seed) % numWorker);
			if (this == &other) {
				return schedule_step();
			}

			// try to steal a task from another queue
			if (TaskBasePtr t = other.queue.pop_front()) {
				t->split();
				t->process();
				// queue.push_back(t);		// add to local queue
				return schedule_step();	// continue scheduling - no stealing
			}

			// no task found => wait a moment
			cpu_relax();
			return false;
		}

	}

	inline void TaskBase::processSubTasks(bool parallel) {
		if (!parallel) {
			// run sequentially
			if (left) left->process();
			if (right) right->process();
			return;
		}

		// run in parallel
		if (left)  runtime::getCurrentWorker().schedule(left);
		if (right) runtime::getCurrentWorker().schedule(right);

		// wait for them to finish
		if (left)  left->wait();
		if (right) right->wait();
	}

	inline void TaskBase::wait() {
		while(!done) {
			// make some progress
			runtime::getCurrentWorker().schedule_step();
		}
	}

	template<typename T>
	void treeture<T>::wait() {
		if (!task) return;
		runtime::getCurrentWorker().schedule(task);
		task->wait();
	}

	void treeture<void>::wait() {
		if(!task) return;

		// narrow scope
		narrow();

		// trigger task execution
		runtime::getCurrentWorker().schedule(task);

		// keep narrowing scope until task fits or task is done
		while(!queue.empty() && !task->isDone()) {
			narrow();
			if (!task->isDone()) runtime::getCurrentWorker().schedule_step();
		}

		// wait for task to be completed
		task->wait();
	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale

