#pragma once

#include <allscale/api/core/impl/simple_old/runtime/lock.h>
#include <atomic>
#include <cstdlib>
#include <thread>

#include "allscale/utils/functional_utils.h"
#include "allscale/utils/printer/arrays.h"


/**
 * This header provides a header-only implementation of a minimal
 * runtime for nested parallel tasks.
 */

namespace allscale {
namespace api {
namespace core {
inline namespace simple {

	struct Worker;

	thread_local static Worker* tl_worker = nullptr;

	static void setCurrentWorker(Worker& worker) {
		tl_worker = &worker;
	}

	static Worker& getCurrentWorker();


	// -----------------------------------------------------------------
	//						Future / Promise
	// -----------------------------------------------------------------

	template<typename T>
	class Future;

	template<>
	class Future<void>;

	template<typename T>
	class Promise;

	template<>
	class Promise<void>;

	template<typename T>
	class FPLink;

	template<>
	class FPLink<void>;


	namespace internal {

		// the kinds of composed futures
		enum class Kind {
			Atomic,
			Sequential,
			Parallel
		};

		template<typename Future, typename Link>
		class FPLinkBase {

			template<typename F, typename L>
			friend
			class FutureBase;

			// the kind of composed future represented
			Kind kind;

			// an atomic reference counter
			std::atomic<int> ref_counter;

			// a flag whether it is complete (if composed, a cache)
			mutable bool done;

			// for composed tasks: the list of sub-tasks
			std::vector<Future> subTasks;

		protected:

			FPLinkBase(bool done = false)
				: kind(Kind::Atomic), ref_counter(1), done(done) {}

			template<typename ... Subs>
			FPLinkBase(Kind kind, Subs&& ... subs)
				: kind(kind), ref_counter(1), done(false), subTasks(sizeof...(subs)) {
				fill(0,std::move(subs)...);
			}

			void fill(unsigned) {}

			template<typename ... Subs>
			void fill(unsigned i, Future&& first, Subs&&  ... rest) {
				subTasks[i] = std::move(first);
				fill(i+1,std::move(rest)...);
			}

			void incRef() {
				ref_counter++;
			}

			void decRef() {
				auto precount = ref_counter.fetch_sub(1);
				if (precount == 1) delete static_cast<Link*>(this);
			}

		public:

			FPLinkBase(const FPLinkBase&) = delete;
			FPLinkBase(FPLinkBase&& other) = delete;

			~FPLinkBase() {
				wait();
			}

			FPLinkBase& operator=(const FPLinkBase& other) = delete;
			FPLinkBase& operator=(FPLinkBase&& other) = delete;

			bool isDone() const {
				if (done) return true;
				if (isAtom()) return false;
				for(const auto& cur : subTasks) {
					if (!cur.isDone()) return false;
				}

				// notify implementation on completion
				static_cast<const Link&>(*this).completed();

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

			void wait() const;

			void markAsDone() {

				// notify implementation on completion
				static_cast<Link&>(*this).completed();

				// mark as done
				done = true;
			}

		};

		template<typename Future, typename Link>
		class FutureBase {

		protected:

			// the future-promise link maintained
			Link* link;

			FutureBase(Link* link) : link(link) {
				link->incRef();
			}

		public:

			FutureBase() : link(nullptr) {}

			FutureBase(const FutureBase&) = delete;

			FutureBase(FutureBase&& other) : link(other.link) {
				other.link = nullptr;
			}

			~FutureBase() {
				if (link) {
					link->wait();
					link->decRef();
				}
			}

			FutureBase& operator=(const FutureBase&) = delete;

			FutureBase& operator=(FutureBase&& other) {
				if (link == other.link) return *this;
				if (link) link->decRef();
				link = other.link;
				other.link = nullptr;
				return *this;
			}

			bool isAtom() const {
				return !link || link->isAtom();
			}

			bool isSequence() const {
				return link && link->isSequence();
			}

			bool isParallel() const {
				return link && link->isParallel();
			}

			bool isComposed() const {
				return link && link->isComposed();
			}

			const std::vector<Future>& getSubTasks() const {
				static const std::vector<Future> none;
				return (link) ? link->getSubTasks() : none;
			}

			void wait() const {
				if (!link) return;
				link->wait();
			}

		};

	}


	template<typename T>
	class FPLink : public internal::FPLinkBase<Future<T>,FPLink<T>> {

		using super = internal::FPLinkBase<Future<T>,FPLink<T>>;

		using aggregator_t = T(*)(const std::vector<Future<T>>&);

		friend class Future<T>;

		friend class Promise<T>;

		friend super;

		aggregator_t aggregator;

		T value;

		FPLink()
			: super(), aggregator(nullptr) {}

		FPLink(const T& value)
			: super(true), aggregator(nullptr), value(value) {}

		template<typename ... Subs>
		FPLink(internal::Kind kind, const aggregator_t& aggregator, Subs&& ... subs)
			: super(kind, std::move(subs)...), aggregator(aggregator) {}

		void setValue(const T& value) {
			this->value = value;
			this->markAsDone();
		}

		const T& getValue() const {
			return value;
		}

		void completed() {
			if (aggregator) {
				value = aggregator(this->getSubTasks());
			}
		}
	};

	template<>
	class FPLink<void> : public internal::FPLinkBase<Future<void>,FPLink<void>> {

		using super = internal::FPLinkBase<Future<void>,FPLink<void>>;

		friend class Future<void>;

		friend class Promise<void>;

		friend super;

		FPLink(bool done = false)
			: super(done) {}

		template<typename ... Subs>
		FPLink(internal::Kind kind, Subs&& ... subs)
			: super(kind, subs...) {}

		void done() {
			this->markAsDone();
		}

		void completed() {
			// nothing to do
		}
	};

	template<typename T>
	class Future : public internal::FutureBase<Future<T>,FPLink<T>> {

		using super = internal::FutureBase<Future<T>,FPLink<T>>;

		friend class Promise<T>;

		Future(FPLink<T>* link) : super(link) {}

	public:

		Future() {}

		Future(const Future&) = delete;

		Future(Future&& other)
			: super(std::move(other)) {}

		Future(const T& res) : super(new FPLink<T>(res)) {}

		template<typename ... Subs>
		Future(internal::Kind kind, const typename FPLink<T>::aggregator_t& aggregator, Subs&& ... subs)
			: super(new FPLink<T>(kind, aggregator, std::move(subs)...)) {}

		Future& operator=(const Future& other) = delete;
		Future& operator=(Future&& other) = default;


		const T& get() const {
			static const T value = T();
			if (!this->link) return value;
			this->wait();
			return this->link->getValue();
		}

		// ---- factories ----

		template<typename S, typename R> friend
		Future<R> atom_internal(const S&);

		template<typename V, typename ... Subs> friend
		Future<V> aggregate(V(*)(const std::vector<Future<V>>&), Subs&& ...);

	};

	template<>
	class Future<void> : public internal::FutureBase<Future<void>,FPLink<void>> {

		using super = internal::FutureBase<Future<void>,FPLink<void>>;

		friend class Promise<void>;

		Future(FPLink<void>* link) : super(link) {}

	public:

		Future() : super() {}

		Future(const Future&) = delete;

		Future(Future&& other)
			: super(std::move(other)) {}

		template<typename ... Subs>
		Future(internal::Kind kind, Subs&& ... subs)
			: super(new FPLink<void>(kind, std::move(subs)...)) {}

		Future& operator=(const Future& other) = delete;
		Future& operator=(Future&& other) = default;

		void get() const {
			this->wait();
		}

		// ---- factories ----

		template<typename S, typename R> friend
		Future<R> atom_internal(const S&);

		template<typename ... Subs> friend
		Future<void> par(Subs&& ...);

		template<typename ... Subs> friend
		Future<void> seq(Subs&& ...);

	};



	template<typename T>
	class Promise {

		FPLink<T>* link;

	public:

		Promise() : link(new FPLink<T>()) {}

		Promise(const Promise& other) : link(other.link) {
			link->incRef();
		}

		Promise(Promise& other) : link(other.link) {
			other.link = nullptr;
		}

		~Promise() {
			if (link) link->decRef();
		}

		Promise& operator==(const Promise& other) = delete;
		Promise& operator==(Promise&& other) = delete;

		Future<T> getFuture() {
			return Future<T>(link);
		}

		void set(const T& res) {
			link->setValue(res);
		}

	};


	template<>
	class Promise<void> {

		FPLink<void>* link;

	public:

		Promise() : link(new FPLink<void>()) {}

		Promise(const Promise& other) : link(other.link) {
			link->incRef();
		}

		Promise(Promise& other) : link(other.link) {
			other.link = nullptr;
		}

		~Promise() {
			if (link) link->decRef();
		}

		Promise& operator==(const Promise& other) = delete;
		Promise& operator==(Promise&& other) = delete;

		Future<void> getFuture() {
			return Future<void>(link);
		}

		void set() {
			link->done();
		}

	};


	// -----------------------------------------------------------------
	//						    Worker Pool
	// -----------------------------------------------------------------

	using Task = std::function<void()>;


	template<typename T, size_t size>
	class SimpleQueue {

		static const int bsize = size + 1;

		SpinLock lock;

		std::array<T,bsize> data;

		size_t front;
		size_t back;

	public:

		SimpleQueue() : lock(), front(0), back(0) {
			for(auto& cur : data) cur = 0;
		}

		bool empty() const {
			return front == back;
		}
		bool full() const {
			return ((back + 1) % bsize) == front;
		}

		bool push_front(const T& t) {
			lock.lock();
			if (full()) {
				lock.unlock();
				return false;
			}
			front = (front - 1 + bsize) % bsize;
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
			back = (back + 1) % bsize;
			lock.unlock();
			return true;
		}

		T pop_front() {
			lock.lock();
			if (empty()) {
				lock.unlock();
				return 0;
			}
			T res = data[front];
			front = (front + 1) % bsize;
			lock.unlock();
			return res;
		}

		T pop_back() {
			lock.lock();
			if (empty()) {
				lock.unlock();
				return 0;
			}
			back = (back - 1 + bsize) % bsize;
			T res = data[back];
			lock.unlock();
			return res;
		}

		friend std::ostream& operator<<(std::ostream& out, const SimpleQueue& queue) {
			return out << "[" << queue.data << "," << queue.front << " - " << queue.back << "]";
		}

	};

	class WorkerPool;

	struct Worker {

		WorkerPool& pool;

		volatile bool alive;

		SimpleQueue<Task*,8> queue;

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

		void run() {

			// register worker
			setCurrentWorker(*this);

			// TODO: idle time handling

			// start processing loop
			while(alive) {
				// conduct a schedule step
				schedule_step();
			}

			// done

		}

	public:

		void schedule_step();

	};




	class WorkerPool {

		std::vector<Worker*> workers;


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
			return getWorker(rand() % workers.size());
		}

	private:

		template<typename Lambda, typename R>
		struct runner {
			Future<R> operator()(Worker& worker, const Lambda& task) {

				// if the queue is full, process task immediately
				if (worker.queue.full()) {
					std::cout << "Direct execution ..\n";
					return task();  // run task and be happy
				}

				// create a schedulable task
				Promise<R> p;
				auto res = p.getFuture();
				Task* t = new Task([=]() mutable {
					std::cout << "Processing in worker ..\n";
					p.set(task());
				});

				// schedule task
				bool succ = worker.queue.push_front(t);
				if (!succ) {
					delete t;
					return task();
				}

				// return future
				return res;
			}
		};

		template<typename Lambda>
		struct runner<Lambda,void> {
			Future<void> operator()(Worker& worker, const Lambda& task) {

				// if the queue is full, process task immediately
				if (worker.queue.full()) {
					std::cout << "Direct execution ..\n";
					task();			// run task and be happy
					return Future<void>();
				}

				// create a schedulable task
				Promise<void> p;
				auto res = p.getFuture();
				Task* t = new Task([=]() mutable {
					task();
					p.set();
				});

				// schedule task
				bool succ = worker.queue.push_front(t);
				if (!succ) {
					delete t;
					task();
					return Future<void>();
				}

				// return future
				return res;

			}
		};

	public:

		template<typename Lambda, typename R>
		Future<R> spawn(const Lambda& lambda) {

			// get current worker
			auto& worker = getCurrentWorker();

			// run task
			return runner<Lambda,R>()(worker, lambda);
		}

		template<typename ... Tasks>
		Future<void> schedule_seq(Tasks&& ... tasks) {

			// TODO: figure out what to do her

		}

		Future<void> schedule_par() {
			return Future<void>();		// job done
		}

		template<typename ... Rest>
		Future<void> schedule_par(Future<void>& first, Rest&& ... rest) {

			// if the queue is full, process tasks immediately
			if (worker.queue.full()) {
				std::cout << "Direct execution ..\n";
				first.wait();	// wait for first task
				return schedule_par(rest);		// wait for rest
			}

			// create a schedulable task
			Promise<void> p;
			auto res = p.getFuture();
			Task* t = new Task([=]() mutable {
				task();
				p.set();
			});

			// schedule task
			bool succ = worker.queue.push_front(t);
			if (!succ) {
				delete t;
				task();
				return Future<void>();
			}

			// return future
			return res;

		}
	};

	namespace internal {

		template<typename Future, typename Link>
		void FPLinkBase<Future,Link>::wait() const {
			if (done) return;
			if (kind == Kind::Atomic) {
				while(!done) {
					// run the scheduler
					getCurrentWorker().schedule_step();
				}
			} else {

				// TODO: enforce sequential ordering

				// wait for all depending tasks
				for(auto& cur : subTasks) {
					cur.wait();
				}
				// conduct post-processing (aggregation)
				const_cast<Link&>(static_cast<const Link&>(*this)).completed();
			}

			done = true;
		}

	}

	inline void Worker::schedule_step() {

		// process a task from the local queue
		if (Task* t = queue.pop_back()) {
			t->operator()();
			delete t;
			return;
		}

		// check that there are other workers
		int numWorker = pool.getNumWorkers();
		if (numWorker <= 1) return;

		// otherwise, steal a task from another worker
		Worker& other = pool.getWorker(rand() % numWorker);
		if (this == &other) {
			schedule_step();
			return;
		}

		if (Task* t = other.queue.pop_front()) {
			t->operator()();
			delete t;
			return;
		}

		// no task found => wait a moment
		cpu_relax();
	}


	static Worker& getCurrentWorker() {
		if (tl_worker) return *tl_worker;
		return WorkerPool::getInstance().getWorker();
	}

	template<
		typename Lambda,
		typename R = typename utils::lambda_traits<Lambda>::result_type
	>
	Future<R> spawn(const Lambda& lambda) {
		return WorkerPool::getInstance().spawn<Lambda,R>(lambda);
	}

	template<typename ... Tasks>
	Future<void> schedule_seq(Tasks&& ... tasks) {
		return WorkerPool::getInstance().schedule_seq(std::move(tasks)...);
	}

	template<typename ... Tasks>
	Future<void> schedule_par(Tasks&& ... tasks) {
		return WorkerPool::getInstance().schedule_par(std::move(tasks)...);
	}

	template<
		typename T,
		typename R,
		typename ... Rest
	>
	Future<R> aggregate(const R(*agg)(const std::vector<Future<T>>&), Future<T>&& first, Rest&& ... rest) {

	}

} // end namespace simple
} // end namespace core
} // end namespace api
} // end namespace allscale
