#pragma once

#include <cassert>
#include <atomic>
#include <memory>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "allscale/utils/assert.h"
#include "allscale/utils/functional_utils.h"
#include "allscale/utils/printer/arrays.h"
#include "allscale/utils/printer/vectors.h"

#include "allscale/api/core/impl/simple/lock.h"


namespace allscale {
namespace api {
namespace core {
inline namespace simple {

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


	// the base class of all futures
	class FutureBase;

	// a future referencing a task producing a value of type T
	template<typename T>
	class Future;

	namespace runtime {

		// the base class of all tasks
		class TaskBase;

		// the generic type of a task computing a value T
		template<typename T>
		class Task;

		// a reference to a task (ref-counted)
		class TaskReference;

		// the wrapper of threads performing actual work
		class Worker;
	}


	// -- Definitions --


	namespace runtime {

		// the kinds of composed tasks
		enum class Kind {
			Atomic,				// < not composed
			Sequential,			// < sequentially processed sub-tasks
			Parallel			// < parallel processed sub-tasks
		};

//		struct RecursiveGroup {
//
//			// the state space of a recursive group
//			enum class State {
//				New,			// < this group is new and just spawning
//				Stable,			// < initial spawning complete
//				Processing,		// < all dependencies are covered, processing
//				Done			// < computation complete
//			};
//
//			State state;						// < the state of this group
//			std::atomic<unsigned> numTasks;		// < the number of tasks created for this group
//		};


		class TaskReference {

			friend runtime::TaskBase;
			friend runtime::Worker;

		public:

			// the task this future is referencing
			runtime::TaskBase* task;

		protected:

			TaskReference(runtime::TaskBase* task);

		public:

			TaskReference()
				: task(nullptr) {}

			TaskReference(const TaskReference& other);

			TaskReference(TaskReference&& other)
				: task(other.task) {
				other.task = nullptr;
			}

			~TaskReference();

			TaskReference& operator=(const TaskReference& other);

			TaskReference& operator=(TaskReference&& other);

			operator bool() const {
				return task;
			}

			bool valid() const {
				return task;
			}

			void wait() const;

			bool isDone() const;

			TaskBase& getTask() const {
				return *task;
			}

			friend std::ostream& operator<<(std::ostream& out, const TaskReference& task) {
				return out << task.task;
			}

		};


		class TaskBase {

			friend class TaskReference;
			friend class Worker;
			friend class simple::FutureBase;


			static unsigned getNextID() {
				static std::atomic<int> counter(0);
				return ++counter;
			}


		public:

			// the state space of tasks
			enum class State {
				New,			// < this task is new
				Ready, 			// < this task may be processed
				Split, 			// < this task has been split
				Aggregating,	// < this split task is aggregating results
				Running, 		// < this task is currently running
				Done			// < this task is done

			};

			friend std::ostream& operator<<(std::ostream& out, const State& state) {
				switch(state) {
					case State::New: 		   return out << "New";
					case State::Ready: 		   return out << "Ready";
					case State::Split: 		   return out << "Split";
					case State::Aggregating:   return out << "Aggregating";
					case State::Running: 	   return out << "Running";
					case State::Done: 		   return out << "Done";
				}
				return out << "Invalid";
			}

//			std::shared_ptr<RecursiveGroup> group;			// < the group this task belongs to

		protected:

			// for debugging -- give each task an ID
			unsigned id;

			// general state
			Kind kind;										// < the type of this task
			std::atomic<State> state;						// < the state of this task
			bool splitable;									// < flag indicating whether has an associated split function

			// data for split tasks
			TaskBase* parent;								// < references the parent owning this instance; if null, there is non
			std::vector<TaskReference> subtasks;			// < futures to sub-tasks if this task is split
			std::atomic<unsigned> alive_child_counter;		// < the number of active children

			// memory management
			std::atomic<unsigned> ref_counter;				// < reference counting for life-time management

		protected:

			TaskBase(bool splitable, State state = State::New)
				: id(getNextID()), kind(Kind::Atomic), state(state), splitable(splitable),
				  parent(nullptr), ref_counter(0) {
				LOG( "Created " << *this );
			}

			template<typename ... Subs>
			TaskBase(Kind kind, Subs&& ... subs);

			virtual ~TaskBase() {
				LOG( "Destroying Task " << *this );
				assert_eq(State::Done, state);
			};

			// -- life cycle management --

			void incRef() {
				ref_counter++;
			}

			void decRef() {
				auto precount = ref_counter.fetch_sub(1);
				if (precount == 1) {
					LOG( "Last reference lost for " << *this );
					delete this;
				}
			}

			// -- state transitions --

			void setState(State newState) {
				state = newState;
				LOG( "Updated state: " << *this );
			}

			// New -> Ready
			void setReady() {
				assert_eq(State::New, state);
				setState(State::Ready);
			}

			// Ready -> Running -> Done
			virtual void execute() =0;

			// New -> Split or Ready -> Split
			virtual void split() =0;

			// Split -> Done
			virtual void aggregate() =0;

			void run() {
				assert_eq(state, State::Ready);

				// update state
				setState(State::Running);

				// run computation
				execute();

				// mark as done
				setDone();
			}

			void wait();

			void childDone(const TaskBase& child) {
				LOG( "Child " << child << " of " << *this << " done" );

				// this one must be a split task
				assert_eq(State::Split, state) << *this;

				// decrement active child count
				unsigned old_child_count = alive_child_counter.fetch_sub(1);

				// log alive counter
				LOG( "Child " << child << " of " << *this << " -- alive left: " << (old_child_count - 1) );

				// check whether this was the last child
				if (old_child_count != 1) return;

				// mark this task as done
				setDone();

				LOG( "Child " << child << " of " << *this << " done - processing complete" );

			}

			// Split -> Done or Running -> Done or Done -> Done
			void setDone() {

				// only process once
				if (state == State::Done) return;

				// check precondition
				assert_true(state == State::Split || state == State::Running)
						<< "Actual State: " << state;

				// if it is a split, let it be done
				State should = State::Split;
				if (state.compare_exchange_strong(should, State::Aggregating)) {

					// log aggregation step
					LOG( "Aggregating task " << *this );

					// aggregate result
					aggregate();

					// clear child list
					subtasks.clear();

					// log completion
					LOG( "Aggregating task " << *this << " complete" );

				}

				// notify parent
				if (parent) {
					parent->childDone(*this);
					parent = nullptr;
				}

				// job is done
				setState(State::Done);

			}

			void moveStateFrom(TaskBase& task);

		public:

			bool isNew() const {
				return state == State::New;
			}

			bool isReady() const {
				return state == State::Ready;
			}

			bool isSplit() const {
				return state == State::Split;
			}

			bool isDone() const {
				return state == State::Done;
			}

			State getState() const {
				return state;
			}

			bool isSplitable() const {
				return splitable && (isNew() || isReady());
			}

			friend std::ostream& operator<<(std::ostream& out, const TaskBase& task) {
				return out << "Task(" << task.id << " @ " << &task << "," << task.state << "," << task.subtasks << ")";;
			}

		};


		template<typename T>
		class Task : public TaskBase {

			friend class Future<T>;

			using aggregator_t = T(*)(const std::vector<Future<T>>&);

			T value;

			std::function<T()> executeOp;				// < to execute the task
			std::function<Future<T>()> splitOp;			// < to split the task
			aggregator_t aggregator;					// < the operation to aggregate results of sub-tasks

		private:

			Task(const std::function<T()>& task)
				: TaskBase(false), executeOp(task), aggregator(nullptr) {}

			Task(const std::function<T()>& task, const std::function<Future<T>()>& split)
				: TaskBase((bool)split), executeOp(task), splitOp(split), aggregator(nullptr) {}

			template<typename ... Subs>
			Task(Kind kind, const aggregator_t& aggregator, Subs&& ... subs)
				: TaskBase(kind, std::move(subs)...), aggregator(aggregator) {}

			Task(const T& value)
				: TaskBase(false,State::Done), value(value), aggregator(nullptr) {}

		protected:

			~Task() override {}

			virtual void execute() override {
				value = executeOp();
			}

			virtual void split() override {
				assert(isSplitable());

				// split this node
				Future<T> sub = splitOp();
				assert(sub.valid());
				Task<T>& task = sub.getTask();

				// copy state important for split node
				moveStateFrom(task);

				// update local state
				aggregator = task.aggregator;

			}

			virtual void aggregate() override {
				// to get a cheap cast into the actual list of future types (they are binary compatible)
				void* tmp = &this->subtasks;
				const std::vector<Future<T>>& subs = *reinterpret_cast<const std::vector<Future<T>>*>(tmp);

				// call the aggregator function
				value = aggregator(subs);
			}


			const T& getValue() const {
				return value;
			}

		public:

			Future<T> getFuture();

			static Future<T> create(const std::function<T()>& task) {
				return (new Task(std::move(task)))->getFuture();
			}

			static Future<T> create(const std::function<T()>& task, const std::function<Future<T>()>& split) {
				return (new Task(std::move(task), std::move(split)))->getFuture();
			}

			template<typename ... Subs>
			static Future<T> create(Kind kind, aggregator_t aggregator, Subs&& ... subs) {
				return (new Task(kind, aggregator, std::move(subs)...))->getFuture();
			}

			static Future<T> create(const T& value) {
				return (new Task(value))->getFuture();
			}

		};

		template<>
		class Task<void> : public TaskBase {

			friend class Future<void>;

			std::function<void()> executeOp;				// < to execute the task
			std::function<Future<void>()> splitOp;			// < to split the task

		private:

			Task(const std::function<void()>& task)
				: TaskBase(false), executeOp(task) {}

			Task(const std::function<void()>& task, const std::function<Future<void>()>& split)
				: TaskBase((bool)split), executeOp(task), splitOp(split) {}

			template<typename ... Subs>
			Task(Kind kind, Subs&& ... subs)
				: TaskBase(kind, std::move(subs)...) {}

		protected:

			~Task() override {}

			virtual void execute() override {
				executeOp();
			}

			virtual void split() override;

			virtual void aggregate() override {
				// nothing to do
			}


		public:

			Future<void> getFuture();

			static Future<void> create(const std::function<void()>& task);

			static Future<void> create(const std::function<void()>& task, const std::function<Future<void>()>& split);

			template<typename ... Subs>
			static Future<void> create(Kind kind, Subs&& ... subs);
		};



		inline TaskReference::TaskReference(runtime::TaskBase* task)
			: task(task) {
			if (task) task->incRef();
		}

		inline TaskReference::TaskReference(const TaskReference& other)
			: task(other.task) {
			if(task) task->incRef();
		}

		inline TaskReference::~TaskReference() {
			if (task) task->decRef();
		}

		inline TaskReference& TaskReference::operator=(const TaskReference& other) {
			if (task == other.task) return *this;
			if (task) task->decRef();
			task = other.task;
			if (task) task->incRef();
			return *this;
		}

		inline TaskReference& TaskReference::operator=(TaskReference&& other) {
			if (task) task->decRef();
			task = other.task;
			other.task = nullptr;
			return *this;
		}

		void TaskReference::wait() const {
			if (!valid()) return;
			getTask().wait();
		}

		bool TaskReference::isDone() const {
			return !valid() || getTask().isDone();
		}

	}



	class FutureBase : public runtime::TaskReference {

		friend runtime::TaskBase;
		friend runtime::Worker;

	protected:

		FutureBase(runtime::TaskBase* task)
			: runtime::TaskReference(task) {}

	public:

		FutureBase() {}

		FutureBase(const FutureBase& other) = delete;

		FutureBase(FutureBase&& other)
			: runtime::TaskReference(std::move(other)) {
		}

		~FutureBase() {
			LOG( "   Destroying future on " << task );
			wait();
		}

		FutureBase& operator=(const FutureBase&) = delete;
		FutureBase& operator=(FutureBase&&) = default;

		template<typename T>
		const Future<T>& asFuture() const {
			return static_cast<const Future<T>&>(*this);
		}

	};

	template<typename T>
	class Future : public FutureBase {

		friend class runtime::Task<T>;

	public:

		Future() = default;
		Future(const Future&) = delete;
		Future(Future&&) = default;

	private:

		Future(runtime::Task<T>* task)
			: FutureBase(task) {};

	public:

		Future& operator=(const Future&) = delete;
		Future& operator=(Future&&) = default;

		const T& get() const {
			static const T fallback = T();
			if (!valid()) return fallback;
			auto& task = getTask();
			task.wait();
			return task.getValue();
		}

	private:

		runtime::Task<T>& getTask() const {
			return static_cast<runtime::Task<T>&>(FutureBase::getTask());
		}

	};

	template<>
	class Future<void> : public FutureBase {

		friend class runtime::Task<void>;

	public:

		Future() = default;
		Future(const Future&) = delete;
		Future(Future&&) = default;

	private:

		Future(runtime::Task<void>* task)
			: FutureBase(task) {};

	public:

		Future& operator=(const Future&) = delete;
		Future& operator=(Future&&) = default;

	private:

		runtime::Task<void>& getTask() const {
			return static_cast<runtime::Task<void>&>(FutureBase::getTask());
		}

	};


	namespace runtime {

		template<typename ... Subs>
		inline TaskBase::TaskBase(Kind kind, Subs&& ... subs)
			: id(getNextID()),
			  kind(kind), state(State::Split), splitable(false),
			  parent(nullptr),
			  subtasks({TaskReference(std::move(subs))...}),
			  alive_child_counter(sizeof...(Subs)),
			  ref_counter(0) {

			// take over ownership
			for(const auto& cur : subtasks) {
				if (cur && !cur.isDone()) {
					cur.task->parent = this;
				}
			}

			LOG( "Created " << *this );
		}

		inline void TaskBase::moveStateFrom(TaskBase& task) {

			// this must only be invoked for a task that is split
			assert_true(isSplitable());

			// the given input task has to be split
			assert_true(task.isSplit() || task.isDone());

			// print some debugging data
			LOG( "Moving state from " << task << " to " << *this );

			// update task state
			kind = task.kind;
			state.store(task.state.load());
			splitable = task.splitable;

			// finish other task
			task.state = State::Done;

			// update sub-tasks
			subtasks.swap(task.subtasks);
			for(const auto& cur : subtasks) {
				cur.task->parent = this;
			}

			// reset child counter (would not need to be atomic)
			alive_child_counter.store(task.alive_child_counter);

			// print some debugging data
			LOG( "Moving state from " << task << " to " << *this << " completed" );

		}


		template<typename T>
		Future<T> Task<T>::getFuture() {
			return Future<T>(this);
		}

		inline Future<void> Task<void>::getFuture() {
			return Future<void>(this);
		}

		inline Future<void> Task<void>::create(const std::function<void()>& task) {
			return (new Task(std::move(task)))->getFuture();
		}

		inline Future<void> Task<void>::create(const std::function<void()>& task, const std::function<Future<void>()>& split) {
			return (new Task(std::move(task), std::move(split)))->getFuture();
		}

		template<typename ... Subs>
		inline Future<void> Task<void>::create(Kind kind, Subs&& ... subs) {
			return (new Task(kind, std::move(subs)...))->getFuture();
		}

		inline void Task<void>::split() {
			assert(isSplitable());

			// split this node
			Future<void> sub = splitOp();
			assert(sub.valid());
			Task<void>& task = sub.getTask();

			// copy state important for split node
			moveStateFrom(task);
		}

	} // end namespace runtime


	namespace runtime {

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

			SpinLock lock;

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
				return (back >= front) ? (back - front) : (buffer_size - (front - back));
			}

			friend std::ostream& operator<<(std::ostream& out, const SimpleQueue& queue) {
				return out << "[" << queue.data << "," << queue.front << " - " << queue.back << "]";
			}

		};

		class WorkerPool;

		struct Worker {

			WorkerPool& pool;

			volatile bool alive;

			SimpleQueue<TaskReference,8> queue;

			std::thread thread;

		public:

			Worker(WorkerPool& pool)
				: pool(pool), alive(true) { }

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

			void schedule(const TaskReference& task);

			void schedule(const std::vector<TaskReference>& tasks);

			bool schedule_step(bool steal = false);

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

				// must be at least one
				if (numWorkers < 1) numWorkers = 1;

				// create workers
				for(int i=0; i<numWorkers; ++i) {
					workers.push_back(new Worker(*this));
				}

				// start workers
				for(auto& cur : workers) cur->start();
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
//				return getWorker(rand() % workers.size());
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

			// register worker
			setCurrentWorker(*this);

			// TODO: idle time handling

			// start processing loop
			while(alive) {
				// conduct a schedule step
				if (!schedule_step(true)) {  	// only top-level conducts stealing
					// there was nothing to do => go to sleep
					pool.waitForWork();
				}
			}

			// done

		}

		void Worker::schedule(const TaskReference& task) {
			schedule(std::vector<TaskReference>({task}));
		}

		void Worker::schedule(const std::vector<TaskReference>& tasks) {

			// add tasks to queue
			unsigned i=0;
			while(i < tasks.size()) {

				LOG_SCHEDULE( "Submitted tasks: " << tasks.size() );
				LOG_SCHEDULE( "Queue size before: " << queue.size() << "/" << queue.capacity );

				// enqueue work as available
				bool enqueuedTasks = false;
				while(i < tasks.size()) {
					// check that it is a non-trivial task
					TaskBase& task = tasks[i].getTask();
					if (!task.isDone()) {
						// mark task as ready
						task.setReady();

						// enqueue task
						if (queue.push_back(tasks[i])) {
							LOG( "Enqueued task " << task << " in task queue" );
							LOG_SCHEDULE( "Enqueued task " << task << " - queue size: " << queue.size() );
							// and remember a task has been enqueued
							enqueuedTasks = true;
						} else {
							break;
						}
					} else {
						LOG_SCHEDULE( "Encountered completed sub-task: " << task );
					}

					// go to next
					i++;
				}

				LOG_SCHEDULE( "Queue size after: " << queue.size() << "/" << queue.capacity );

				// signal available work
				if (enqueuedTasks && queue.size() > queue.capacity/2) {
					pool.workAvailable();
				}

				// process rest directly
				if (i < tasks.size()) {

					// process the task directly -- no queues, no split
					TaskBase& task = tasks[i].getTask();

					// check state of task
					assert_true(task.isNew() || task.isReady());

					LOG( "Running directly " << task );

					// if not already marked as ready => do it now
					if (!task.isReady()) task.setReady();

					// run task
					task.run();

					// make sure task is done
					assert_true(task.isDone()) << "Actual state: " << task.getState();

					// continue with next task
					i++;
				}
			}
		}


		inline bool Worker::schedule_step(bool steal) {

			// process a task from the local queue
			if (TaskReference t = queue.pop_front()) {

				LOG( "Processing " << t.getTask() );
				LOG_SCHEDULE( "Processing " << t.getTask() << " - queue size: " << queue.size() );

				// if the queue is not full => create more tasks
				if (queue.size() < (queue.capacity*3)/4 && t.task->isSplitable()) {

					LOG_SCHEDULE( "Splitting " << t.getTask() << " - queue size: " << queue.size() );

					// split task
					t.task->split();

					// process the split task
					t.task->wait();		// process split task

					// done
					return true;
				}

				// if the queue is full => work on this task
				LOG( "Running " << t.getTask() );
				LOG_SCHEDULE( "Running " << t.getTask() );

				t.getTask().run();		// triggers execution
				return true;
			}

			// if no stealing should be conducted => we are done
			if (!steal) return false;

			// check that there are other workers
			int numWorker = pool.getNumWorkers();
			if (numWorker <= 1) return false;

			// otherwise, steal a task from another worker
			Worker& other = pool.getWorker(rand() % numWorker);
			if (this == &other) {
				return schedule_step(steal);
			}

			// try to steal a task from another queue
			if (TaskReference t = other.queue.pop_front()) {
				queue.push_back(t);		// add to local queue
				LOG( "Stole " << *t.task << " from other worker" );
				return schedule_step();	// continue scheduling - no stealing
			}

			// no task found => wait a moment
			cpu_relax();
			return false;
		}


		inline void TaskBase::wait() {

			// keep this task alive while being processed
			TaskReference ref(this);

			assert_true(state >= State::New && state <= State::Done);
			LOG( "Waiting for " << *this );

			// if this task is already done => done
			if (state == State::Done) {
				LOG( "   - waiting for " << *this << " completed - quick" );
				return;
			}

			// compute task
			if (state == State::Split) {

				// submit child tasks for processing
				if (kind == Kind::Parallel) {

					// start processing of parallel tasks

					// get current worker
					Worker& worker = getCurrentWorker();

					// get a copy of the sub-tasks (since they are deleted after aggregation)
					std::vector<TaskReference> subs = subtasks;

					// enqueue tasks
					worker.schedule(subs);

					// wait for tasks to complete
					for(const auto& cur : subs) {
						while(!isDone() && !cur.isDone()) {
							worker.schedule_step();
						}
					}

				} else {

					// get a copy of the sub-tasks (since they are deleted after aggregation)
					std::vector<TaskReference> subs = subtasks;

					// process sequentially
					for(const auto& cur : subs) {
						cur.wait();
					}

				}

				// now this task is complete
				LOG( "Waited for all subtasks of " << *this << " - finishing this task" );
				setDone();

			} else if (state == State::New) {

				// get current worker
				Worker& worker = getCurrentWorker();

				// enqueue tasks
				worker.schedule(this);

				// wait for completion
				while(!isDone()) {
					worker.schedule_step();
				}
			}

			LOG( "   - waiting for " << *this << " completed" );

		}

	}


	template<typename R>
	Future<R> done(const R& value) {
		return runtime::Task<R>::create(value);
	}

	inline Future<void> done() {
		return Future<void>();
	}

	template<
		typename T,
		typename R = typename utils::lambda_traits<T>::result_type
	>
	Future<R> spawn(const T& task) {
		return runtime::Task<R>::create(task);
	}

	template<
		typename E,
		typename S,
		typename RE = typename utils::lambda_traits<E>::result_type,
		typename RS = typename utils::lambda_traits<S>::result_type
	>
	typename std::enable_if<
		std::is_same<Future<RE>,RS>::value,
		Future<RE>
	>::type
	spawn(const E& task, const S& split) {
		return runtime::Task<RE>::create(task,split);
	}

	// -- composed futures --

	template<typename V, typename ... Subs>
	Future<V> aggregate(V(* aggregator)(const std::vector<Future<V>>&), Subs&& ... subs) {
		return runtime::Task<V>::create(runtime::Kind::Parallel, aggregator, std::move(subs)... );
	}

	template<typename ... Subs>
	Future<void> par(Subs&& ... subs) {
		return runtime::Task<void>::create(runtime::Kind::Parallel, std::move(subs)... );
	}

	template<typename ... Subs>
	Future<void> seq(Subs&& ... subs) {
		return runtime::Task<void>::create(runtime::Kind::Sequential, std::move(subs)... );
	}

} // end namespace simple
} // end namespace core
} // end namespace api
} // end namespace allscale
