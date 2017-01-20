#pragma once

#include <array>
#include <atomic>
#include <algorithm>
#include <bitset>
#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <type_traits>

#include <pthread.h>

#include "allscale/utils/assert.h"
#include "allscale/utils/printer/arrays.h"

#include "allscale/api/core/impl/reference/lock.h"
#include "allscale/api/core/impl/reference/profiling.h"
#include "allscale/api/core/impl/reference/runtime_predictor.h"

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	// ------------------------------------- Declarations -----------------------------------------

	/**
	 * The actual treeture, referencing the computation of a value.
	 */
	template<typename T>
	class treeture;

	/**
	 * A treeture not yet released to the runtime system for execution.
	 */
	template<typename T>
	class unreleased_treeture;

	/**
	 * A handle for a lazily constructed unreleased treeture. This intermediate construct is utilized
	 * for writing templated code that can be optimized to overhead-less computed values and to facilitate
	 * the support of the sequence combinator.
	 */
	template<typename T, typename Gen>
	class lazy_unreleased_treeture;

	/**
	 * A reference to a task to synchronize upon it.
	 */
	class task_reference;

	/**
	 * A class to model task dependencies
	 */
	using dependencies = std::vector<task_reference>;



	// ---------------------------------------------------------------------------------------------
	//								    Internal Forward Declarations
	// ---------------------------------------------------------------------------------------------


	class TaskBase;

	template<typename T>
	class Task;

	using TaskBasePtr = std::shared_ptr<TaskBase>;

	template<typename T>
	using TaskPtr = std::shared_ptr<Task<T>>;

	using TaskDependencies = dependencies;


	// ---------------------------------------------------------------------------------------------
	//											  Debugging
	// ---------------------------------------------------------------------------------------------


	// -- Declarations --

	const bool DEBUG = false;

	std::mutex g_log_mutex;

	#define LOG(MSG) \
		{  \
			if (DEBUG) { \
				std::thread::id this_id = std::this_thread::get_id(); \
				std::lock_guard<std::mutex> lock(g_log_mutex); \
				std::cerr << "Thread " << this_id << ": " << MSG << "\n"; \
			} \
		}

	const bool DEBUG_SCHEDULE = false;

	#define LOG_SCHEDULE(MSG) \
		{  \
			if (DEBUG_SCHEDULE) { \
				std::thread::id this_id = std::this_thread::get_id(); \
				std::lock_guard<std::mutex> lock(g_log_mutex); \
				std::cerr << "Thread " << this_id << ": " << MSG << "\n"; \
			} \
		}

	const bool DEBUG_TASKS = false;

	#define LOG_TASKS(MSG) \
		{  \
			if (DEBUG_TASKS) { \
				std::thread::id this_id = std::this_thread::get_id(); \
				std::lock_guard<std::mutex> lock(g_log_mutex); \
				std::cerr << "Thread " << this_id << ": " << MSG << "\n"; \
			} \
		}



	// -----------------------------------------------------------------
	//						  Monitoring (for Debugging)
	// -----------------------------------------------------------------


	const bool ENABLE_MONITORING = false;

	namespace monitoring {

		enum class EventType {
			Run, Split, Wait
		};

		struct Event {

			EventType type;

			const TaskBase* task;

			bool operator==(const Event& other) const {
				return other.type == type && other.task == task;
			}

			friend std::ostream& operator<<(std::ostream& out, const Event& e);
		};


		class ThreadState {

			using guard = std::lock_guard<std::mutex>;

			std::thread::id thread_id;

			std::mutex lock;

			std::vector<Event> eventStack;

		public:

			ThreadState() : thread_id(std::this_thread::get_id()) {
				guard g(getStateLock());
				getStates().push_back(this);
			}

			~ThreadState() {
				assert_true(eventStack.empty());

				guard g(getStateLock());
				auto& list = getStates();
				auto it = std::find(list.begin(),list.end(),this);
				if (it != list.end()) {
					std::swap(*it,list.back());
					list.pop_back();
				}
			}

			void pushEvent(const Event& e) {
				guard g(lock);
				eventStack.push_back(e);
			}

			void popEvent(__unused const Event& e) {
				guard g(lock);
				assert_eq(e,eventStack.back());
				eventStack.pop_back();
			}

			void dumpState(std::ostream& out) {
				guard g(lock);
				out << "\nThread: " << thread_id << "\n";
				out << "\tStack:\n";
				for(const auto& cur : eventStack) {
					out << "\t\t" << cur << "\n";
				}
				out << "\n";
			}

			static void dumpStates(std::ostream& out) {
				// lock states
				std::lock_guard<std::mutex> g(getStateLock());

				// print all current states
				for(const auto& cur : getStates()) {
					cur->dumpState(out);
				}
			}

		private:

			static std::mutex& getStateLock() {
				static std::mutex state_lock;
				return state_lock;
			}

			static std::vector<ThreadState*>& getStates() {
				static std::vector<ThreadState*> states;
				return states;
			}

		};

		thread_local static ThreadState tl_thread_state;


		struct Action {

			bool active;
			Event e;

			Action() : active(false) {}

			Action(const Event& e) : active(true), e(e) {
				// register action
				tl_thread_state.pushEvent(e);
			}

			Action(Action&& other) : active(other.active), e(other.e) {
				other.active = false;
			}

			Action(const Action&) = delete;

			Action& operator=(const Action&) = delete;
			Action& operator=(Action&&) = delete;

			~Action() {
				if (!active) return;
				// remove action from action stack
				tl_thread_state.popEvent(e);
			}

		};

		Action log(EventType type, const TaskBase* task) {
			if (!ENABLE_MONITORING) return {};
			return Event{type,task};
		}

	}


	// ---------------------------------------------------------------------------------------------
	//											  Tasks
	// ---------------------------------------------------------------------------------------------



	// the RT's interface to a task
	class TaskBase : public std::enable_shared_from_this<TaskBase> {

		static unsigned getNextID() {
			static std::atomic<int> counter(0);
			return (DEBUG || DEBUG_SCHEDULE || DEBUG_TASKS || ENABLE_MONITORING || PROFILING_ENABLED) ? ++counter : 0;
		}

	public:

		enum class State {
			New,          // < this task has been created, but not processed by a worker yet
			Blocked,      // < this task has unfinished dependencies
			Ready,        // < this task may be processed (scheduled in work queues)
			Running,      // < this task is running
			Aggregating,  // < this split task is aggregating results (skipped if not split)
			Done          // < this task is completed
		};

		friend std::ostream& operator<<(std::ostream& out, const State& state) {
			switch(state) {
				case State::New:           return out << "New";
				case State::Blocked:       return out << "Blocked";
				case State::Ready:         return out << "Ready";
				case State::Running:       return out << "Running";
				case State::Aggregating:   return out << "Aggregating";
				case State::Done:          return out << "Done";
			}
			return out << "Invalid";
		}

	private:

		// for debugging -- give each task an ID
		TaskID id;

		// the current state of this task
		std::atomic<State> state;

		// a list of task to be completed before this task can be started
		TaskDependencies dependencies;

		// indicates whether this task can be split
		bool splitable;

		// split task data
		TaskBasePtr left;
		TaskBasePtr right;
		bool parallel;

		// for the processing of split tasks
		TaskBase* parent;                      // < a pointer to the parent to be notified upon completion
		std::atomic<int> alive_child_counter;  // < the number of active child tasks

		// for the mutation from a simple to a split task
		TaskBasePtr substitute;

	public:

		TaskBase(TaskDependencies&& deps = TaskDependencies(), bool done = true)
			: id(getNextID()), state(done ? State::Done : State::New), dependencies(std::move(deps)), splitable(false), parent(nullptr) {
			LOG_TASKS( "Created " << *this );
		}

		TaskBase(TaskDependencies&& dependencies, TaskBasePtr&& left, TaskBasePtr&& right, bool parallel)
			: id(getNextID()), state(State::New),
			  dependencies(std::move(dependencies)),
			  left(std::move(left)), right(std::move(right)), parallel(parallel),
			  parent(nullptr), alive_child_counter(0) {
			LOG_TASKS( "Created " << *this );
			assert(this->left);
			assert(this->right);
		}

		virtual ~TaskBase() {
			LOG_TASKS( "Destroying Task " << *this );
			assert_true(isDone()) << *this;
		};

		// -- observers --

		TaskID getId() const {
			return id;
		}

		void setId(const TaskID& id) {
			this->id = id;
			if (substitute) substitute->setId(id);
			if (left) left->setId(id.getLeftChild());
			if (right) right->setId(id.getRightChild());
		}

		std::size_t getDepth() const {
			return id.getDepth();
		}

		State getState() const {
			return state;
		}

		std::vector<TaskBasePtr> getDependencies() const;


		// -- state transitions --

		// New -> Blocked
		void start();

		// Blocked -> Blocked or Blocked -> Ready transition
		bool isReady();

		// Ready -> Running -> Done
		void run() {
			// log this event
			auto action = monitoring::log(monitoring::EventType::Run, this);

			LOG_TASKS( "Running Task " << *this );

			// check that it is allowed to run
			assert_eq(state, State::Ready);

			// update state
			setState(State::Running);

			// forward call to substitute if present
			if (substitute) {
				if (!substitute->isDone()) {

					// run substitute
					substitute->run();

					// wait for the substitute to be finish
					substitute->wait();

					// at this point, the job should be done as well
					assert_true(isDone());

				} else {
					finish();
				}

				return;
			}

			// process split tasks
			if (isSplit()) {					// if there is a left, it is a split task

				// check some assumptions
				assert(left && right);
				assert( left->state == State::New ||  left->state == State::Done);
				assert(right->state == State::New || right->state == State::Done);

				// run task sequentially if requested
				if (!parallel) {
					left->runInline();
					right->runInline();
					finish();
					return;
				}

				// count number of sub-tasks to be started
				assert_eq(0,alive_child_counter);
				if (left->state  == State::New) alive_child_counter++;
				if (right->state == State::New) alive_child_counter++;

				// check whether there are sub-tasks
				if (alive_child_counter == 0) {
					// perform reduction immediately since sub-tasks are done
					finish();
					// done
					return;
				}

				// backup right since if left finishes quickly it will be reset
				TaskBasePtr r = right;

				// schedule left (if necessary)
				if (left->state == State::New) {
					LOG_TASKS( "Starting child " << *left << " of " << *this );
					left->parent = this;
					left->start();
				}

				// schedule right (if necessary)
				if (r->state == State::New) {
					LOG_TASKS( "Starting child " << *r << " of " << *this );
					r->parent = this;
					r->start();
				}

				// wait until this task is completed
				wait();

				// done
				return;
			}

			// check that all dependencies have completed
			assert_true(std::all_of(dependencies.begin(),dependencies.end(),[](const auto& cur){
				return cur.isDone();
			}));

			// run computation
			execute();

			// finish task
			finish();
		}

		void runInline() {
			// forward call to substitute if present
			if (substitute) {
				substitute->runInline();
				return;
			}

			// skip done tasks
			if (isDone()) return;

			// set to ready without going through the worker pool
			setState(TaskBase::State::Ready);

			// run the task
			run();
		}

		// Ready -> Split (if supported, otherwise remains Ready)
		virtual void split() {
			// log this event
			auto action = monitoring::log(monitoring::EventType::Split, this);

			// forward call to substitute if present
			if (substitute) {
				substitute->split();
				return;
			}

			// this task must not be running yet
			assert_lt(state,State::Running);

			// by default, no splitting is supported
		}

		// wait for the task completion
		void wait();

		bool isDone() const {
			// forward call to substitute if present
			if (substitute) return substitute->isDone();
			return state == State::Done;
		}

		TaskBasePtr getLeft() const {
			// forward call to substitute if present
			if (substitute) return substitute->getLeft();
			return left;
		}

		TaskBasePtr getRight() const {
			// forward call to substitute if present
			if (substitute) return substitute->getRight();
			return right;
		}

		bool isSplitable() const {
			return splitable;
		}

		bool isSplit() const {
			return (bool)left;
		}

	protected:

		// Ready -> Running -> Done
		virtual void execute() =0;

		// Split -> Done
		virtual void aggregate() {
			// nothing to do by default
		}

		void setSplitable(bool value = true) {
			splitable = value;
		}

		void setSubstitute(TaskBasePtr&& newSub) {
			// can only happen if this task is in blocked or ready state
			assert_true(state == State::Blocked || state == State::Ready)
					<< "Actual state: " << state;

			// must only be set once!
			assert_false(substitute);
			assert_true(newSub);
			substitute = std::move(newSub);

			// update id of substitute
			substitute->setId(this->getId());

			// enable sub-task (bring task to ready state if necessary, without scheduling)
			if (substitute->state == TaskBase::State::New) substitute->setState(TaskBase::State::Ready);

			// connect substitute to parent
			substitute->parent = this;
		}

	private:

		bool isValidTransition(State from, State to) {
			return (from == State::New         && to == State::Ready       ) ||
				   (from == State::New         && to == State::Blocked     ) ||
				   (from == State::Blocked     && to == State::Ready       ) ||
				   (from == State::Ready       && to == State::Running     ) ||
				   (from == State::Running     && to == State::Done        ) ||
				   (from == State::Running     && to == State::Aggregating ) ||
				   (from == State::Aggregating && to == State::Done        ) ;
		}

		void setState(State newState) {
			// check correctness of state transitions
			assert_true(isValidTransition(state,newState))
				<< "Illegal state transition from " << state << " to " << newState;

			state = newState;
			LOG_TASKS( "Updated state: " << *this );
		}

		bool switchState(State from, State to) {
			assert_true(isValidTransition(from,to))
				<< "Illegal state transition from " << from << " to " << to;
			return state.compare_exchange_strong(from,to,std::memory_order_relaxed,std::memory_order_relaxed);
		}

		void childDone(const TaskBase& child) {

			// check whether it is the substitute
			if (substitute.get() == &child) {
				LOG_TASKS( "Substitute " << substitute << " of " << *this << " done");
				finish();
				return;
			}

			// process a split-child
			LOG_TASKS( "Child " << child << " of " << *this << " done" );

			// this one must be a split task
			assert_true(isSplit());
			assert_true(left.get() == &child || right.get() == &child) << *this;

			// decrement active child count
			unsigned old_child_count = alive_child_counter.fetch_sub(1);

			// log alive counter
			LOG_TASKS( "Child " << child << " of " << *this << " -- alive left: " << (old_child_count - 1) );

			// check whether this was the last child
			if (old_child_count != 1) return;

			// finish this task
			finish();

			// LOG_TASKS( "Child " << child << " of " << *this << " done - processing complete" );
		}

		void finish() {

			LOG_TASKS( "Finishing task " << *this );

			// check precondition
			assert_true(state == State::Running)
					<< "Actual State: " << state << "\nTask: " << *this;

			// if it is a split, finish it by aggregating results
			if (isSplit()) {
				// update state
				setState(State::Aggregating);

				// log aggregation step
				LOG( "Aggregating task " << *this );

				// aggregate result
				aggregate();

				// TODO: figure out a way to clean up memory safely
//				// delete sub-tasks
//				left.reset();
//				right.reset();

				// log completion
				LOG( "Aggregating task " << *this << " complete" );
			}

			// job is done
			setState(State::Done);

			// notify parent
			if (parent) {
				parent->childDone(*this);
			}
		}

		// -- support printing of tasks for debugging --

		friend std::ostream& operator<<(std::ostream& out, const TaskBase& task) {

			auto printDependencies = [&]() {
				out << "{" << utils::join(",",task.getDependencies(),[](std::ostream& out, const TaskBasePtr& p){
					out << *p;
				}) << "}";
			};

			// if substituted, print the task and its substitute
			if (task.substitute) {
				out << task.id << " ";
				printDependencies();
				out << " -> " << *task.substitute;
				return out;
			}

			// if split, print the task and its children
			if (task.isSplit()) {
				out << task.id << " ";
				printDependencies();
				out << " : " << task.state << " = [";
				if (task.left) out << *task.left; else out << "nil";
				out << ",";
				if (task.right) out << *task.right; else out << "nil";
				out << "] ";
				return out;
			}

			// in all other cases, just print the id
			out << task.id << " ";
			printDependencies();
			out << " : " << task.state;
			return out;
		}

		template<typename Process, typename Split, typename R>
		friend class SplitableTask;

	};


	// a task computing a value of type T
	template<typename T>
	class Task : public TaskBase {

		T value;

		Task<T>* substitute;

	public:

		Task(TaskDependencies&& deps) : TaskBase(std::move(deps),false), substitute(nullptr) {}

		Task(TaskDependencies&& deps, const T& value)
			: TaskBase(std::move(deps),true), value(value), substitute(nullptr) {}

		Task(TaskDependencies&& deps, TaskBasePtr&& left, TaskBasePtr&& right, bool parallel)
			: TaskBase(std::move(deps),std::move(left),std::move(right), parallel), substitute(nullptr) {}


		virtual ~Task(){};

		const T& getValue() const {
			if (substitute) return substitute->getValue();
			return value;
		}

	protected:

		void execute() override {
			value = computeValue();
		}

		void aggregate() override {
			value = computeAggregate();
		}

		virtual T computeValue() { return value; };
		virtual T computeAggregate() { return value; };

		void setSubstitute(TaskPtr<T>&& substitute) {
			this->substitute = substitute.get();
			TaskBase::setSubstitute(std::move(substitute));
		}

	};

	template<>
	class Task<void> : public TaskBase {
	public:

		Task(TaskDependencies&& deps) : TaskBase(std::move(deps),false) {}

		Task(TaskDependencies&& deps, TaskBasePtr&& left, TaskBasePtr&& right, bool parallel)
			: TaskBase(std::move(deps),std::move(left),std::move(right),parallel) {}

		virtual ~Task(){};

		void getValue() const {
		}

	protected:

		void execute() override {
			computeValue();
		}

		void aggregate() override {
			computeAggregate();
		}

		virtual void computeValue() {};
		virtual void computeAggregate() {};

	};


	template<
		typename Process,
		typename R = std::result_of_t<Process()>
	>
	class SimpleTask : public Task<R> {

		Process task;

	public:

		SimpleTask(TaskDependencies&& deps, const Process& task)
			: Task<R>(std::move(deps)), task(task) {}

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

		SplitableTask(TaskDependencies&& deps, const Process& c, const Split& d)
			: Task<R>(std::move(deps)), task(c), decompose(d) {
			// mark this task as one that can be split
			TaskBase::setSplitable();
		}

		R computeValue() override {
			// if split
			if (subTask) {
				// compute sub-task
				subTask->runInline();
				return subTask->getValue();
			}
			return task();
		}

		void split() override;

		R computeAggregate() override {
			assert(subTask);
			return subTask->getValue();
		}

	};

	template<typename R, typename A, typename B, typename C>
	class SplitTask : public Task<R> {

		Task<A>& left;
		Task<B>& right;

		C merge;

	public:

		SplitTask(TaskDependencies&& deps, TaskPtr<A>&& left, TaskPtr<B>&& right, C&& merge, bool parallel)
			: Task<R>(std::move(deps),std::move(left),std::move(right),parallel),
			  left(static_cast<Task<A>&>(*this->getLeft())),
			  right(static_cast<Task<B>&>(*this->getRight())),
			  merge(merge) {}


		R computeValue() override {
			// should not be reached
			assert_fail() << "Should always be split!";
			return {};
		}

		// Split -> Done
		R computeAggregate() override {
			return merge(left.getValue(),right.getValue());
		}

	};

	template<typename A, typename B>
	class SplitTask<void,A,B,void> : public Task<void> {
	public:

		SplitTask(TaskDependencies&& deps, TaskBasePtr&& left, TaskBasePtr&& right, bool parallel)
			: Task<void>(std::move(deps),std::move(left),std::move(right),parallel) {}

		void computeValue() override {
			// should not be reached
			assert_fail() << "Should always be split!";
		}

		// Split -> Done
		void computeAggregate() override {
			// nothing to do
		}

	};

	template<typename A, typename B, typename C, typename R = std::result_of_t<C(A,B)>>
	TaskPtr<R> make_split_task(TaskDependencies&& deps, TaskPtr<A>&& left, TaskPtr<B>&& right, C&& merge, bool parallel) {
		return std::make_shared<SplitTask<R,A,B,C>>(std::move(deps), std::move(left), std::move(right), std::move(merge), parallel);
	}

	inline TaskPtr<void> make_split_task(TaskDependencies&& deps, TaskBasePtr&& left, TaskBasePtr&& right, bool parallel) {
		return std::make_shared<SplitTask<void,void,void,void>>(std::move(deps), std::move(left), std::move(right), parallel);
	}



	// ---------------------------------------------------------------------------------------------
	//										task reference
	// ---------------------------------------------------------------------------------------------


	/**
	 * A simple buffer of left/right decisions in the navigation of a treeture and
	 * its sub-tasks.
	 */
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


	/**
	 * A reference to a task utilized for managing task synchronization.
	 */
	class task_reference {

		template<typename Process, typename Split, typename R>
		friend class SplitableTask;

	protected:

		mutable TaskBasePtr task;

	private:

		mutable BitQueue queue;

		mutable SpinLock lock;

	public:

		task_reference() {}

		task_reference(TaskBasePtr&& task) : task(std::move(task)) {}

		task_reference(const TaskBasePtr& task) : task(task) {}

		task_reference(const task_reference& other) {
			std::lock_guard<SpinLock> lease(other.lock);
			task = other.task;
			queue = other.queue;
		}

		task_reference(task_reference&& other) {
			std::lock_guard<SpinLock> lease(other.lock);
			task = std::move(other.task);
			queue = std::move(other.queue);
		}

		task_reference& operator=(const task_reference&) = delete;

		task_reference& operator=(task_reference&& other) {
			if (this == &other) return *this;
			std::lock_guard<SpinLock> leaseA(lock);
			std::lock_guard<SpinLock> leaseB(other.lock);
			task = std::move(other.task);
			queue = std::move(other.queue);
			return *this;
		}

		bool isDone() const {
			narrow();
			std::lock_guard<SpinLock> lease(lock);
			if (!task) return true;
			if (!task->isDone()) return false;
			task.reset();
			return true;
		}

		void wait() const;

		task_reference& descentLeft() {
			std::lock_guard<SpinLock> lease(lock);
			if (!task) return *this;
			queue.put(0);
			return *this;
		}

		task_reference& descentRight() {
			std::lock_guard<SpinLock> lease(lock);
			if (!task) return *this;
			queue.put(1);
			return *this;
		}

		task_reference getLeft() const {
			return task_reference(*this).descentLeft();
		}

		task_reference getRight() const {
			return task_reference(*this).descentRight();
		}

		operator TaskBasePtr() const {
			narrow();
			return task;
		}

	private:

		void narrow() const {
			std::lock_guard<SpinLock> lease(lock);
			if (!task) return;
			while(!task->isDone() && !queue.empty()) {

				// get sub-task (if available)
				TaskBasePtr next = (queue.get()) ? task->getRight() : task->getLeft();

				// if not available, that is the closet we can get for now
				if (!next) return;

				// narrow the task reference
				queue.pop();
				task = next;
			}
		}

	};


	// ---------------------------------------------------------------------------------------------
	//											Treetures
	// ---------------------------------------------------------------------------------------------


	namespace detail {

		/**
		 * A common base class for all treetures, providing common functionality.
		 */
		template<typename T>
		class treeture_base {

			template<typename Process, typename Split, typename R>
			friend class SplitableTask;

		protected:

			TaskBasePtr task;

			treeture_base(TaskBasePtr&& task) : task(std::move(task)) {}

		public:

			using value_type = T;

			treeture_base(const treeture_base&) = delete;
			treeture_base(treeture_base&& other) : task(std::move(other.task)) {}

			treeture_base& operator=(const treeture_base&) = delete;
			treeture_base& operator=(treeture_base&& other) {
				task = std::move(other.task);
				return *this;
			}

			void wait() const {
				if (!task) return;
				task->wait();
			}

			task_reference getLeft() const {
				return task_reference(task).descentLeft();
			}

			task_reference getRight() const {
				return task_reference(task).descentRight();
			}

			task_reference getTaskReference() const {
				return task_reference(task);
			}

			operator task_reference() const {
				return getTaskReference();
			}

		};

	}

	/**
	 * A treeture, providing a reference to the state of a task as well as to
	 * the computed value upon completion.
	 */
	template<typename T>
	class treeture : public detail::treeture_base<T> {

		using super = detail::treeture_base<T>;

		friend class unreleased_treeture<T>;

	protected:

		treeture(TaskPtr<T>&& task) : super(std::move(task)) {}

	public:

		treeture(const T& value = T()) : super(std::make_shared<Task<T>>(TaskDependencies(),value)) {}

		treeture(const treeture&) = delete;
		treeture(treeture&& other) = default;

		treeture& operator=(const treeture&) = delete;
		treeture& operator=(treeture&& other) = default;

		const T& get() {
			super::wait();
			return std::static_pointer_cast<Task<T>>(this->task)->getValue();
		}

	};

	/**
	 * A specialization of the general value treeture for the void type, exhibiting
	 * a modified signature for the get() member function.
	 */
	template<>
	class treeture<void> : public detail::treeture_base<void> {

		using super = detail::treeture_base<void>;

		friend class unreleased_treeture<void>;

	protected:

		treeture(TaskBasePtr&& task) : super(std::move(task)) {}

	public:

		treeture() : super(TaskBasePtr()) {}

		treeture(const treeture&) = delete;
		treeture(treeture&& other) = default;

		treeture& operator=(const treeture&) = delete;
		treeture& operator=(treeture&& other) = default;

		void get() {
			wait();
		}

	};



	template<typename Process, typename Split, typename R>
	void SplitableTask<Process,Split,R>::split() {
		// do not split a second time
		if (!TaskBase::isSplitable()) return;

		assert_true(TaskBase::State::Blocked == this->state || TaskBase::State::Ready == this->state)
				<< "Actual state: " << this->state;

		// decompose this task
		subTask = decompose().toTask();
		assert_true(subTask);
		assert_true(subTask->state == TaskBase::State::New || subTask->state == TaskBase::State::Done);

		// mutate to new task
		Task<R>::setSubstitute(std::move(subTask));

		// mark as no longer splitable
		TaskBase::setSplitable(false);

		// also, mark as ready for being processed
		if (this->state != TaskBase::State::Ready) {
			TaskBase::setState(TaskBase::State::Ready);
		}
	}



	// ---------------------------------------------------------------------------------------------
	//										 Unreleased Treetures
	// ---------------------------------------------------------------------------------------------


	/**
	 * A handle to a yet unreleased task.
	 */
	template<typename T>
	class unreleased_treeture : public task_reference {
	public:

		using value_type = T;

		unreleased_treeture(TaskPtr<T>&& task)
			: task_reference(std::move(task)) {}

		unreleased_treeture(const unreleased_treeture&) =delete;
		unreleased_treeture(unreleased_treeture&&) =default;

		unreleased_treeture& operator=(const unreleased_treeture&) =delete;
		unreleased_treeture& operator=(unreleased_treeture&&) =default;

		~unreleased_treeture() { if(task) assert_ne(TaskBase::State::New,task->getState()); }

		treeture<T> release() && {
			if (task && !task->isDone()) task->start(); 		// release the task
			return treeture<T>(std::move(*this).toTask());
		}

		operator treeture<T>() && {
			return std::move(*this).release();
		}

		T get() && {
			return std::move(*this).release().get();
		}

		TaskPtr<T> toTask() && {
			auto res = std::move(std::static_pointer_cast<Task<T>>(std::move(task)));
			task.reset();
			return res;
		}

	};



	// ---------------------------------------------------------------------------------------------
	//										   Operators
	// ---------------------------------------------------------------------------------------------



	inline dependencies after() {
		return dependencies();
	}

	template<typename ... Rest>
	dependencies after(const task_reference& r, const Rest& ... rest) {
		auto res = after(std::move(rest)...);
		res.push_back(r);
		return res;
	}

	dependencies after(const std::vector<task_reference>& refs) {
		return refs;
	}


	inline unreleased_treeture<void> done(dependencies&& deps) {
		return std::make_shared<Task<void>>(std::move(deps));
	}

	inline unreleased_treeture<void> done() {
		return done(dependencies());
	}

	template<typename T>
	unreleased_treeture<T> done(dependencies&& deps, const T& value) {
		return std::make_shared<Task<T>>(std::move(deps),value);
	}

	template<typename T>
	unreleased_treeture<T> done(const T& value) {
		return done(dependencies(),value);
	}


	template<typename Action, typename T = std::result_of_t<Action()>>
	unreleased_treeture<T> spawn(dependencies&& deps, Action&& op) {
		return TaskPtr<T>(std::make_shared<SimpleTask<Action>>(std::move(deps),std::move(op)));
	}

	template<typename Action>
	auto spawn(Action&& op) {
		return spawn(after(),std::move(op));
	}

	template<typename Action, typename Split, typename T = std::result_of_t<Action()>>
	unreleased_treeture<T> spawn(dependencies&& deps, Action&& op, Split&& split) {
		return TaskPtr<T>(std::make_shared<SplitableTask<Action,Split>>(std::move(deps),std::move(op),std::move(split)));
	}

	template<typename Action, typename Split>
	auto spawn(Action&& op, Split&& split) {
		return spawn(after(),std::move(op),std::move(split));
	}

	inline unreleased_treeture<void> sequential(dependencies&& deps) {
		return done(std::move(deps));
	}

	inline unreleased_treeture<void> sequential() {
		return done();
	}

	template<typename A, typename B>
	unreleased_treeture<void> sequential(dependencies&& deps, unreleased_treeture<A>&& a, unreleased_treeture<B>&& b) {
		return make_split_task(std::move(deps),std::move(a).toTask(),std::move(b).toTask(),false);
	}

	template<typename A, typename B>
	unreleased_treeture<void> sequential(unreleased_treeture<A>&& a, unreleased_treeture<B>&& b) {
		return sequential(after(),std::move(a),std::move(b));
	}

	template<typename F, typename ... R>
	unreleased_treeture<void> sequential(dependencies&& deps, unreleased_treeture<F>&& f, unreleased_treeture<R>&& ... rest) {
		// TODO: conduct a binary split to create a balanced tree
		return make_split_task(std::move(deps),std::move(f).toTask(),sequential(std::move(rest)...).toTask(),false);
	}

	template<typename F, typename ... R>
	unreleased_treeture<void> sequential(unreleased_treeture<F>&& f, unreleased_treeture<R>&& ... rest) {
		return sequential(after(), std::move(f),std::move(rest)...);
	}


	inline unreleased_treeture<void> parallel(dependencies&& deps) {
		return done(std::move(deps));
	}

	inline unreleased_treeture<void> parallel() {
		return done();
	}

	template<typename A, typename B>
	unreleased_treeture<void> parallel(dependencies&& deps, unreleased_treeture<A>&& a, unreleased_treeture<B>&& b) {
		return make_split_task(std::move(deps),std::move(a).toTask(),std::move(b).toTask(),true);
	}

	template<typename A, typename B>
	unreleased_treeture<void> parallel(unreleased_treeture<A>&& a, unreleased_treeture<B>&& b) {
		return parallel(after(),std::move(a),std::move(b));
	}

	template<typename F, typename ... R>
	unreleased_treeture<void> parallel(dependencies&& deps, unreleased_treeture<F>&& f, unreleased_treeture<R>&& ... rest) {
		// TODO: conduct a binary split to create a balanced tree
		return make_split_task(std::move(deps),std::move(f).toTask(),parallel(std::move(deps),std::move(rest)...).toTask(),true);
	}

	template<typename F, typename ... R>
	unreleased_treeture<void> parallel(unreleased_treeture<F>&& f, unreleased_treeture<R>&& ... rest) {
		return parallel(after(), std::move(f),std::move(rest)...);
	}



	template<typename A, typename B, typename M, typename R = std::result_of_t<M(A,B)>>
	unreleased_treeture<R> combine(dependencies&& deps, unreleased_treeture<A>&& a, unreleased_treeture<B>&& b, M&& m, bool parallel = true) {
		return make_split_task(std::move(deps),std::move(a).toTask(),std::move(b).toTask(),std::move(m),parallel);
	}

	template<typename A, typename B, typename M, typename R = std::result_of_t<M(A,B)>>
	unreleased_treeture<R> combine(unreleased_treeture<A>&& a, unreleased_treeture<B>&& b, M&& m, bool parallel = true) {
		return reference::combine(dependencies(),std::move(a),std::move(b),std::move(m),parallel);
	}


	// ---------------------------------------------------------------------------------------------
	//											Runtime
	// ---------------------------------------------------------------------------------------------

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

			using guard = std::lock_guard<SpinLock>;

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
				guard g(lock);
				if (full()) {
					return false;
				}
				front = (front - 1 + buffer_size) % buffer_size;
				data[front] = t;
				return true;
			}

			bool push_back(const T& t) {
				guard g(lock);
				if (full()) {
					return false;
				}
				data[back] = t;
				back = (back + 1) % buffer_size;
				return true;
			}

		private:

			T pop_front_internal() {
				if (empty()) {
					return T();
				}
				T res(std::move(data[front]));
				front = (front + 1) % buffer_size;
				return res;
			}

			T pop_back_internal() {
				if (empty()) {
					return T();
				}
				back = (back - 1 + buffer_size) % buffer_size;
				T res(std::move(data[back]));
				return res;
			}

		public:

			T pop_front() {
				guard g(lock);
				return pop_front_internal();
			}

			T try_pop_front() {
				if (!lock.try_lock()) {
					return {};
				}
				const T& res = pop_front_internal();
				lock.unlock();
				return res;
			}

			T pop_back() {
				guard g(lock);
				return pop_back_internal();
			}

			T try_pop_back() {
				if (!lock.try_lock()) {
					return {};
				}
				const T& res = pop_back_internal();
				lock.unlock();
				return res;
			}

			size_t size() const {
				guard g(lock);
				return (back >= front) ? (back - front) : (buffer_size - (front - back));
			}

			std::vector<T> getSnapshot() const {
				std::vector<T> res;
				guard g(lock);
				size_t i = front;
				while(i != back) {
					res.push_back(data[i]);
					i += (i + 1) % buffer_size;
				}
				return res;
			}

			friend std::ostream& operator<<(std::ostream& out, const SimpleQueue& queue) {
				guard g(queue.lock);
				return out << "[" << queue.data << "," << queue.front << " - " << queue.back << "]";
			}

		};

		class SimpleTaskPool {

			using guard = std::lock_guard<SpinLock>;

			std::vector<TaskBasePtr> pool;

			mutable SpinLock lock;

		public:

			bool empty() const {
				guard lease(lock);
				return pool.empty();
			}

			void addTask(TaskBasePtr&& task) {
				guard lease(lock);
				pool.emplace_back(std::move(task));
			}

			TaskBasePtr getReadyTask() {
				guard lease(lock);

				// 1) look for some task that is ready
				for(auto& cur : pool) {
					if(!cur->isReady()) continue;
					// found a ready one, remove it from the pool
					TaskBasePtr res = std::move(cur);
					cur = std::move(pool.back());
					pool.pop_back();
					return res;
				}

				// 2) look for the largest splitable task
				auto min = std::numeric_limits<std::size_t>::max();
				for(auto& cur : pool) {
					if (!cur->isSplitable()) continue;
					min = std::min(cur->getDepth(), min);
				}

				// 3) split the largest splitable task and return it
				for(auto& cur : pool) {
					if (cur->getDepth() != min || !cur->isSplitable()) continue;

					// found a splitable one, split it
					TaskBasePtr res = std::move(cur);
					cur = std::move(pool.back());
					pool.pop_back();

					// split the task and return the result
					res->split();

					assert_true(res->isReady());
					return res;
				}

				// no work available
				return {};
			}

			std::vector<TaskBasePtr> getSnapshot() const {
				guard g(lock);
				std::vector<TaskBasePtr> res = pool;
				return res;
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

			using duration = RuntimePredictor::duration;

			WorkerPool& pool;

			volatile bool alive;

			// list of tasks ready to run
			SimpleQueue<TaskBasePtr,16> queue;

			// list of blocked tasks
			SimpleTaskPool& blocked;

			std::thread thread;

			unsigned id;

			unsigned random_seed;

			RuntimePredictor predictions;

		public:

			Worker(WorkerPool& pool, SimpleTaskPool& blockedTasks, unsigned id)
				: pool(pool), alive(true), blocked(blockedTasks), id(id), random_seed(id) { }

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

			void dumpState(std::ostream& out) const {
				out << "Worker " << id << " / " << thread.get_id() << ":\n";
				out << "\tQueue:\n";
				for(const auto& cur : queue.getSnapshot()) {
					out << "\t\t" << *cur << "\n";
				}

				out << "\tBlocked:\n";
				for(const auto& cur : blocked.getSnapshot()) {
					out << "\t\t" << *cur << " waiting for [";
					out << utils::join(",", cur->getDependencies(), [](std::ostream& out, const TaskBasePtr& dep) {
						out << dep->getId() << ":" << dep->getState();
					}) << "]\n";
				}
			}

		private:

			void run();

			void runTask(const TaskBasePtr& task);

			void splitTask(const TaskBasePtr& task);

			duration estimateRuntime(const TaskBasePtr& task) {
				return predictions.predictTime(task->getDepth());
			}

		public:

			void schedule(TaskBasePtr&& task);

			bool schedule_step();

		};

		class WorkerPool {

			std::vector<Worker*> workers;

			// tools for managing idle threads
			std::mutex m;
			std::condition_variable cv;

			// a global pool of blocked tasks
			SimpleTaskPool blockedTasks;

		public:

			WorkerPool() {

				int numWorkers = std::thread::hardware_concurrency();

				// parse environment variable
				if (char* val = std::getenv("NUM_WORKERS")) {
					auto userDef = std::atoi(val);
					if (userDef != 0) numWorkers = userDef;
				}

				// there must be at least one worker
				if (numWorkers < 1) numWorkers = 1;

				// create workers
				for(int i=0; i<numWorkers; ++i) {
					workers.push_back(new Worker(*this,blockedTasks,i));
				}

				// start additional workers (worker 0 is main thread)
				for(int i=1; i<numWorkers; ++i) {
					workers[i]->start();
				}

				// make worker 0 being linked to the main thread
				setCurrentWorker(*workers.front());

				// fix affinity of main thread
				detail::fixAffinity(0);

				// fix worker id of main thread
				setCurrentWorkerID(0);

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
				for(std::size_t i=1; i<workers.size(); ++i) {
					workers[i]->join();
				}

				// free resources
				for(auto& cur : workers) {
					delete cur;
				}

			}

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

			void dumpState(std::ostream& out) {
				for(const auto& cur : workers) {
					cur->dumpState(out);
				}
			}

		protected:

			friend Worker;

			void waitForWork() {
				std::unique_lock<std::mutex> lk(m);
				LOG_SCHEDULE("Going to sleep");
				cv.wait(lk);
				LOG_SCHEDULE("Woken up again");
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

			// fix worker ID
			setCurrentWorkerID(id);

			// log creation of worker event
			logProfilerEvent(ProfileLogEntry::createWorkerCreatedEntry());

			// fix affinity
			detail::fixAffinity(id);

			// register worker
			setCurrentWorker(*this);

			// start processing loop
			while(alive) {

				// count number of idle cycles
				int idle_cycles = 0;

				// conduct a schedule step
				while(alive && !schedule_step()) {
					// increment idle counter
					++idle_cycles;

					// wait a moment
					cpu_relax();

					// if there was no work for quite some time
					if(idle_cycles > 100000) {
						// if there may be no work at all ..
						if(blocked.empty()) {

							// report sleep event
							logProfilerEvent(ProfileLogEntry::createWorkerSuspendedEntry());

							// wait for work by putting thread to sleep
							pool.waitForWork();

							// report awakening
							logProfilerEvent(ProfileLogEntry::createWorkerResumedEntry());
						}
				
						// reset cycles counter
						idle_cycles = 0;
					}
				}
			}

			// log worker termination event
			logProfilerEvent(ProfileLogEntry::createWorkerDestroyedEntry());

			// done

		}

		void Worker::runTask(const TaskBasePtr& task) {
			LOG_SCHEDULE("Starting task " << task);
			logProfilerEvent(ProfileLogEntry::createTaskStartedEntry(task->getId()));

			if (task->isSplit()) {
				task->run();
			} else {
				// take the time to make predictions
				auto start = std::chrono::high_resolution_clock::now();
				task->run();
				auto time = std::chrono::high_resolution_clock::now() - start;
				predictions.registerTime(task->getDepth(),time);
			}

			logProfilerEvent(ProfileLogEntry::createTaskEndedEntry(task->getId()));
			LOG_SCHEDULE("Finished task " << task);
		}

		void Worker::splitTask(const TaskBasePtr& task) {
			using namespace std::chrono_literals;

			// the threshold for estimated task to be split
			static auto taskTimeThreshold = 1ms;

			// only split the task if it is estimated to exceed a threshold
			if (task->isSplitable() && estimateRuntime(task) > taskTimeThreshold) {
				task->split();
			}
		}

		inline void Worker::schedule(TaskBasePtr&& task) {

			// check whether task has unfinished dependencies
			if (!task->isReady()) {
				blocked.addTask(std::move(task));
				return;
			}

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
			runTask(task);
		}


		inline bool Worker::schedule_step() {

			// process a task from the local queue
			if (TaskBasePtr t = queue.pop_front()) {

				// if the queue is not full => create more tasks
				if (queue.size() < (queue.capacity*3)/4) {

					LOG_SCHEDULE( "Splitting tasks @ queue size: " << queue.size() << "/" << queue.capacity );

					// split task
					splitTask(t);
				}

				// process this task
				runTask(t);
				return true;
			}

			// no task in queue, check the pool
			if (TaskBasePtr t = blocked.getReadyTask()) {

				LOG_SCHEDULE( "Retrieving unblocked task from list of blocked tasks.");

				// split task the task (since there is not enough work in the queue)
				splitTask(t);

				// process this task
				runTask(t);
				return true;	// successfully completed a task
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
			if (TaskBasePtr t = other.queue.try_pop_front()) {

				// log creation of worker event
				logProfilerEvent(ProfileLogEntry::createTaskStolenEntry(t->getId()));

				LOG_SCHEDULE( "Stolen task: " << t );

				// split task the task (since there is not enough work in the queue)
				splitTask(t);

				// process task
				runTask(t);
				return true;	// successfully completed a task
			}

			// no task found => wait a moment
			cpu_relax();
			return false;
		}

	}

	namespace monitoring {

		inline std::ostream& operator<<(std::ostream& out, const Event& e) {
			switch(e.type) {
			case EventType::Run:    return out << "Running task     " << *e.task;
			case EventType::Split:  return out << "Splitting task   " << *e.task;
			case EventType::Wait:   return out << "Waiting for task " << *e.task;
			}
			return out << "Unknown Event";
		}

	}// end namespace monitoring

	std::vector<TaskBasePtr> TaskBase::getDependencies() const {
		std::vector<TaskBasePtr> res;
		for(const auto& cur : dependencies) {
			res.push_back(cur);
		}
		return res;
	}

	void TaskBase::start() {
		LOG_TASKS("Starting " << *this );

		// check that the given task is a new task
		assert_eq(TaskBase::State::New, state);

		// move to next state
		setState(State::Blocked);

		// schedule task
		runtime::getCurrentWorker().schedule(this->shared_from_this());
	}


	inline bool TaskBase::isReady() {
		// should only be called if blocked or ready
		assert_true(state == State::Blocked || state == State::Ready) << "Actual State:" << state;

		// if substituted, check substitute
		if (substitute) {
			return substitute->isReady();
		}

		// if already identified as ready before, that's it
		if (state == State::Ready) return true;

		// upgrade new to blocked
		if (state == State::New) setState(State::Blocked);

		// check if there are alive dependencies
		for(auto& cur : dependencies) {
			if (!cur.isDone()) return false;
		}

		LOG_TASKS( "Preconditions satisfied, task ready: " << *this );

		// Update state from new to ready
		setState(State::Ready);

		// free up dependency vector
		dependencies.clear();

		// return result
		return true;
	}

	inline void TaskBase::wait() {
		// log this event
		auto action = monitoring::log(monitoring::EventType::Wait, this);

		LOG_TASKS("Waiting for " << *this );

		// check that this task has been started before
		assert_lt(State::New,state);

		// forward call to substitute, if present
		if (substitute) {
			substitute->wait();
			return;
		}

		while(!isDone()) {
			// make some progress
			runtime::getCurrentWorker().schedule_step();
		}
	}

	inline void task_reference::wait() const {
		// keep narrowing scope until done (implicit in isDone)
		while(!isDone()) {
			runtime::getCurrentWorker().schedule_step();
		}
	}

} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale


void __dumpRuntimeState() {
	allscale::api::core::impl::reference::monitoring::ThreadState::dumpStates(std::cout);
	allscale::api::core::impl::reference::runtime::WorkerPool::getInstance().dumpState(std::cout);
}


