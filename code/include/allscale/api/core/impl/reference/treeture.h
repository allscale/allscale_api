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
	//								 	Task Dependency Manager
	// ---------------------------------------------------------------------------------------------

	template<std::size_t max_depth>
	class TaskDependencyManager {

		struct Entry {
			TaskBasePtr task;
			Entry* next;
		};

		using cell_type = std::atomic<Entry*>;

		enum { num_entries = 1<<(max_depth+1) };

		cell_type data[num_entries];

	public:

		TaskDependencyManager() {
			for(auto& cur : data) cur = nullptr;
		}

		~TaskDependencyManager() {
			for(auto& cur : data) {
				if (!isDone(cur)) delete cur;
			}
		}

		TaskDependencyManager(const TaskDependencyManager&) = delete;
		TaskDependencyManager(TaskDependencyManager&&) = delete;

		TaskDependencyManager& operator=(const TaskDependencyManager&) = delete;
		TaskDependencyManager& operator=(TaskDependencyManager&&) = delete;


		/**
		 * Adds a dependency between the given tasks such that
		 * task x depends on the completion of the task y.
		 */
		void addDependency(const TaskBasePtr& x, const TaskPath& y);

		void markComplete(const TaskPath& task);

		bool isComplete(const TaskPath& path) const {
			return isDone(data[getPosition(path)]);
		}

	private:

		std::size_t getPosition(const TaskPath& path) const {
			std::size_t res = 1;
			for(const auto& cur : path) {
				res = res * 2 + cur;
				if (res >= num_entries) return res / 2;
			}
			return res;
		}

		bool isDone(const Entry* ptr) const {
			// if the last bit is set, the task already finished
			return (intptr_t)(ptr) & 0x1;
		}

	};



	// ---------------------------------------------------------------------------------------------
	//											 Task Family
	// ---------------------------------------------------------------------------------------------


	/**
	 * A task family is a collection of tasks descending from a common (single) ancestor.
	 * Task families are created by root-level prec operator calls, and manage the dependencies
	 * of all its members.
	 *
	 * Tasks being created through recursive or combine calls are initially not members of
	 * any family, but may get adapted (by being the result of a split operation).
	 */
	class TaskFamily {

		using DependencyManager = TaskDependencyManager<12>;

		// the unique ID of this family (for debugging)
		std::size_t id;

		// the manager of all dependencies on members of this family
		DependencyManager dependencies;

	public:

		/**
		 * Creates a new family, using a new ID.
		 */
		TaskFamily() : id(getNextID()) {}

		/**
		 * Obtain the family ID.
		 */
		std::size_t getId() const {
			return id;
		}

		/**
		 * Tests whether the given sub-task is complete.
		 */
		bool isComplete(const TaskPath& path) const {
			return dependencies.isComplete(path);
		}

		/**
		 * Register a dependency ensuring that a task x is depending on a task y.
		 */
		void addDependency(const TaskBasePtr& x, const TaskPath& y) {
			dependencies.addDependency(x,y);
		}

		/**
		 * Mark the given task as being finished.
		 */
		void markDone(const TaskPath& x) {
			dependencies.markComplete(x);
		}

	private:

		static unsigned getNextID() {
			static std::atomic<int> counter(0);
			return (DEBUG || DEBUG_SCHEDULE || DEBUG_TASKS || ENABLE_MONITORING || PROFILING_ENABLED) ? ++counter : 0;
		}

	};

	// the pointer type to reference task families
	using TaskFamilyPtr = std::shared_ptr<TaskFamily>;

	// a factory for a new task family
	TaskFamilyPtr createFamily() {
		return std::make_shared<TaskFamily>();
	}



	// ---------------------------------------------------------------------------------------------
	//										task reference
	// ---------------------------------------------------------------------------------------------


	/**
	 * A reference to a task utilized for managing task synchronization. Tasks may
	 * only be synchronized on if they are members of a task family.
	 */
	class task_reference {

		TaskFamilyPtr family;

		TaskPath path;

		task_reference(const TaskFamilyPtr& family, const TaskPath& path)
			: family(family), path(path) {}

	public:

		task_reference() : path(TaskPath::root()) {}

		task_reference(const TaskBasePtr& task);

		bool isDone() const {
			return (!family || family->isComplete(path));
		}

		void wait() const;

		task_reference getLeft() const {
			return task_reference ( family, path.getLeftChildPath() );
		}

		task_reference getRight() const {
			return task_reference ( family, path.getRightChildPath() );
		}

		task_reference& descentLeft() {
			path.descentLeft();
			return *this;
		}

		task_reference& descentRight() {
			path.descentRight();
			return *this;
		}

		// -- implementation details --

		const TaskFamilyPtr& getFamily() const {
			return family;
		}

		const TaskPath& getPath() const {
			return path;
		}

	};



	// ---------------------------------------------------------------------------------------------
	//											  Tasks
	// ---------------------------------------------------------------------------------------------



	// the RT's interface to a task
	class TaskBase : public std::enable_shared_from_this<TaskBase> {

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

		// the family this task belongs to, if null, this task is an orphan task.
		TaskFamilyPtr family;

		// the position of this task within its family
		TaskPath path;

		// the current state of this task
		std::atomic<State> state;

		// the number of tasks still active before this tasks can be released
		std::atomic<std::size_t> num_active_dependencies;

		// indicates whether this task can be split
		bool splitable;

		// split task data
		TaskBasePtr left;
		TaskBasePtr right;
		bool parallel;

		// for the processing of split tasks
		TaskBasePtr parent;                    // < a pointer to the parent to be notified upon completion
		std::atomic<int> alive_child_counter;  // < the number of active child tasks

		// for the mutation from a simple to a split task
		TaskBasePtr substitute;

	public:

		TaskBase(bool done = true)
			: family(), path(),
			  state(done ? State::Done : State::New),
			  // one initial control flow dependency, released by treeture release
			  num_active_dependencies(1),
			  splitable(false), parallel(false), parent(nullptr) {

			LOG_TASKS( "Created " << *this );
		}

		TaskBase(TaskBasePtr&& left, TaskBasePtr&& right, bool parallel)
			: family(),
			  path(),
			  state(State::New),
			  // one initial control flow dependency, released by treeture release
			  num_active_dependencies(1),
			  splitable(false),
			  left(std::move(left)), right(std::move(right)), parallel(parallel),
			  parent(nullptr), alive_child_counter(0) {

			LOG_TASKS( "Created " << *this );
			assert(this->left);
			assert(this->right);
		}

		virtual ~TaskBase() {
			LOG_TASKS( "Destroying Task " << *this );
			assert_true(isDone()) << getId() << " - " << getState();
		};

		// -- observers --

		const TaskFamilyPtr& getTaskFamily() const {
			return family;
		}

		const TaskPath& getTaskPath() const {
			return path;
		}

		TaskID getId() const {
			auto familyId = (family) ? family->getId() : 0;
			return TaskID(familyId,path);
		}

		bool isOrphan() const {
			return !family;
		}

		std::size_t getDepth() const {
			return path.getLength();
		}

		State getState() const {
			return state;
		}


		// -- mutators --

		void addDependencies(const TaskDependencies& dependencies) {

			// ignore empty dependencies
			if (dependencies.empty()) return;

			// we must still be in the new state
			assert_eq(getState(),State::New);

			// increase the number of active dependencies
			num_active_dependencies += dependencies.size();

			// register dependencies
			auto thisTask = this->shared_from_this();
			for(const auto& cur : dependencies) {

				// filter out already completed tasks (some may be orphans)
				if (cur.isDone()) {
					// notify that one dependency more is completed
					dependencyDone();
					// continue with next
					continue;
				}

				// add dependency
				assert_true(cur.getFamily());
				cur.getFamily()->addDependency(thisTask,cur.getPath());
			}

		}

		void adopt(const TaskFamilyPtr& family, const TaskPath& path = TaskPath()) {
			// check that this task is not member of another family
			assert_true(isOrphan()) << "Can not adopt a member of another family.";

			// check whether there is an actual family
			if (!family) return;

			// join the family
			this->family = family;
			this->path = path;

			// mark as complete, if already complete
			if(isDone()) family->markDone(path);

			// propagate adoption to descendants
			if (substitute) substitute->adopt(family,path);
			if (left)  left->adopt(family, path.getLeftChildPath());
			if (right) right->adopt(family, path.getRightChildPath());
		}


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

				// get a shared pointer to this task to be passed as a parent
				TaskBasePtr thisTask = this->shared_from_this();

				// backup right since if left finishes quickly it will be reset
				TaskBasePtr r = right;

				// schedule left (if necessary)
				if (left->state == State::New) {
					LOG_TASKS( "Starting child " << *left << " of " << *this );
					left->parent = thisTask;
					left->start();
				}

				// schedule right (if necessary)
				if (r->state == State::New) {
					LOG_TASKS( "Starting child " << *r << " of " << *this );
					r->parent = thisTask;
					r->start();
				}

				// wait until this task is completed
				wait();

				// done
				return;
			}

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

		void dependencyDone();

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

			// adapt substitute
			substitute->adopt(this->family, this->path);

			// enable sub-task (bring task to ready state if necessary, without scheduling)
			if (substitute->state == TaskBase::State::New) substitute->setState(TaskBase::State::Ready);

			// connect substitute to parent
			substitute->parent = this->shared_from_this();
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

			// mark as done
			if (family) family->markDone(path);

			// notify parent
			if (parent) {
				parent->childDone(*this);
				parent.reset();
			}
		}

		// -- support printing of tasks for debugging --

		friend std::ostream& operator<<(std::ostream& out, const TaskBase& task) {

			// if substituted, print the task and its substitute
			if (task.substitute) {
				out << task.getId() << " -> " << *task.substitute;
				return out;
			}

			// if split, print the task and its children
			if (task.isSplit()) {
				out << task.getId() << " : " << task.state << " = [";
				if (task.left) out << *task.left; else out << "nil";
				out << ",";
				if (task.right) out << *task.right; else out << "nil";
				out << "] ";
				return out;
			}

			// in all other cases, just print the id
			out << task.getId() << " : " << task.state;

			std::size_t numDependencies = task.num_active_dependencies;
			if (numDependencies > 0) {
				std::cout << " waiting for " << numDependencies << " task(s)";
			}

			return out;
		}

		template<typename Process, typename Split, typename R>
		friend class SplitableTask;

	};


	// ----------- Task Dependency Manager Implementations ---------------

	template<std::size_t max_depth>
	void TaskDependencyManager<max_depth>::addDependency(const TaskBasePtr& x, const TaskPath& y) {

		// locate entry
		std::size_t pos = getPosition(y);

		// load the head
		Entry* head = data[pos].load(std::memory_order_relaxed);

		// check whether this task is already completed
		if (isDone(head)) {
			// signal that this dependency is done
			x->dependencyDone();
			return;
		}

		// insert element
		Entry* entry = new Entry();
		entry->task = x;
		entry->next = head;

		// update entry pointer lock-free
		while (!data[pos].compare_exchange_weak(entry->next,entry, std::memory_order_relaxed, std::memory_order_relaxed)) {

			// check whether the task has been completed in the meanwhile
			if (isDone(entry->next)) {
				delete entry;
				// signal that this dependency is done
				x->dependencyDone();
				return;
			}

			// otherwise, repeat until it worked
		}

		// successfully inserted
	}

	template<std::size_t max_depth>
	void TaskDependencyManager<max_depth>::markComplete(const TaskPath& task) {

		// ignore tasks that are too small
		if (task.getLength() > max_depth) return;

		// mark as complete and obtain head of depending list
		auto pos = getPosition(task);
		Entry* cur = data[pos].exchange((Entry*)0x1, std::memory_order_relaxed);

		// do not process list twice (may be called multiple times due to substitutes)
		if (isDone(cur)) return;

		// signal the completion of this task
		while(cur) {

			// signal a completed dependency
			cur->task->dependencyDone();

			// move on to next entry
			Entry* next = cur->next;
			delete cur;
			cur = next;
		}

		// and its children
		if (pos >= num_entries/2) return;
		markComplete(task.getLeftChildPath());
		markComplete(task.getRightChildPath());
	}

	// -------------------------------------------------------------------



	// ------------------------- Task Reference --------------------------

	task_reference::task_reference(const TaskBasePtr& task)
		: family(task->getTaskFamily()), path(task->getTaskPath()) {
		assert_true(family) << "Unable to reference an orphan task!";
	}

	// -------------------------------------------------------------------


	// a task computing a value of type T
	template<typename T>
	class Task : public TaskBase {

		T value;

		Task<T>* substitute;

	public:

		Task() : TaskBase(false), substitute(nullptr) {}

		Task(const T& value)
			: TaskBase(true), value(value), substitute(nullptr) {}

		Task(TaskBasePtr&& left, TaskBasePtr&& right, bool parallel)
			: TaskBase(std::move(left),std::move(right), parallel), substitute(nullptr) {}


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

		Task() : TaskBase(false) {}

		Task(TaskBasePtr&& left, TaskBasePtr&& right, bool parallel)
			: TaskBase(std::move(left),std::move(right),parallel) {}

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

		SimpleTask(const Process& task)
			: Task<R>(), task(task) {}

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
			: Task<R>(), task(c), decompose(d) {
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

		SplitTask(TaskPtr<A>&& left, TaskPtr<B>&& right, C&& merge, bool parallel)
			: Task<R>(std::move(left),std::move(right),parallel),
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

		SplitTask(TaskBasePtr&& left, TaskBasePtr&& right, bool parallel)
			: Task<void>(std::move(left),std::move(right),parallel) {}

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
		auto res = std::make_shared<SplitTask<R,A,B,C>>(std::move(left), std::move(right), std::move(merge), parallel);
		res->addDependencies(deps);
		return res;
	}

	inline TaskPtr<void> make_split_task(TaskDependencies&& deps, TaskBasePtr&& left, TaskBasePtr&& right, bool parallel) {
		auto res = std::make_shared<SplitTask<void,void,void,void>>(std::move(left), std::move(right), parallel);
		res->addDependencies(deps);
		return res;
	}





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

		treeture(const T& value = T()) : super(std::make_shared<Task<T>>(value)) {}

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
	class unreleased_treeture {

		TaskPtr<T> task;

	public:

		using value_type = T;

		unreleased_treeture(TaskPtr<T>&& task)
			: task(std::move(task)) {}

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
		auto res = std::make_shared<Task<void>>();
		res->addDependencies(deps);
		return std::move(res);
	}

	inline unreleased_treeture<void> done() {
		return done(dependencies());
	}

	template<typename T>
	unreleased_treeture<T> done(dependencies&& deps, const T& value) {
		auto res = std::make_shared<Task<T>>(value);
		res->addDependencies(deps);
		return std::move(res);
	}

	template<typename T>
	unreleased_treeture<T> done(const T& value) {
		return done(dependencies(),value);
	}


	namespace detail {

		template<bool root, typename T>
		unreleased_treeture<T> init(dependencies&& deps, TaskPtr<T>&& task) {

			// add dependencies
			task->addDependencies(deps);

			// create task family if requested
			if (root) {
				task->adopt(createFamily());
			}

			// done
			return std::move(task);
		}

	}


	template<bool root, typename Action, typename T = std::result_of_t<Action()>>
	unreleased_treeture<T> spawn(dependencies&& deps, Action&& op) {
		// create and initialize the task
		return detail::init<root>(std::move(deps), TaskPtr<T>(std::make_shared<SimpleTask<Action>>(std::move(op))));
	}

	template<bool root, typename Action>
	auto spawn(Action&& op) {
		return spawn<root>(after(),std::move(op));
	}

	template<bool root, typename Action, typename Split, typename T = std::result_of_t<Action()>>
	unreleased_treeture<T> spawn(dependencies&& deps, Action&& op, Split&& split) {
		// create and initialize the task
		return detail::init<root>(std::move(deps), TaskPtr<T>(std::make_shared<SplitableTask<Action,Split>>(std::move(op),std::move(split))));
	}

	template<bool root, typename Action, typename Split>
	auto spawn(Action&& op, Split&& split) {
		return spawn<root>(after(),std::move(op),std::move(split));
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

			std::thread thread;

			unsigned id;

			unsigned random_seed;

			RuntimePredictor predictions;

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

			void dumpState(std::ostream& out) const {
				out << "Worker " << id << " / " << thread.get_id() << ":\n";
				out << "\tQueue:\n";
				for(const auto& cur : queue.getSnapshot()) {
					out << "\t\t" << *cur << "\n";
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
					workers.push_back(new Worker(*this,i));
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

						// report sleep event
						logProfilerEvent(ProfileLogEntry::createWorkerSuspendedEntry());

						// wait for work by putting thread to sleep
						pool.waitForWork();

						// report awakening
						logProfilerEvent(ProfileLogEntry::createWorkerResumedEntry());
				
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

			// assert that this task is ready to run
			assert_true(task->isReady());

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
			if (!task->isReady()) return;

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

				// check precondition of task
				assert_true(t->isReady());

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


	void TaskBase::start() {
		LOG_TASKS("Starting " << *this );

		// check that the given task is a new task
		assert_eq(TaskBase::State::New, state);

		// move to next state
		setState(State::Blocked);

		// split tasks by default up to a given level
//		// TODO: make this depth hardware dependent
//		if (!isOrphan() && isSplitable() && getDepth() < 4) {
//
//			// split this task
//			split();
//
//			// now it should be ready
//			assert_true(isReady());
//		}

		// release dummy-dependency to get task started
		dependencyDone();
	}

	void TaskBase::dependencyDone() {
		// decrease number of active dependencies
		assert_gt(num_active_dependencies, 0);

		// decrease the number of active dependencies
		if (num_active_dependencies.fetch_sub(1) != 1) return;

		// this was the last dependency => enqueue task if blocked
		if (state != State::Blocked) return;


		// TODO: actively distribute initial tasks, by assigning
		// them to different workers;

		// enqueue task
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

		// check if there are alive dependencies
		if (num_active_dependencies != 0) return false;

		LOG_TASKS( "Preconditions satisfied, task ready: " << *this );

		// Update state from new to ready
		setState(State::Ready);

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


