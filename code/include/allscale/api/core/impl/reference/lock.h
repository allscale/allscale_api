#pragma once

#include <atomic>
#include <thread>

namespace allscale {
namespace api {
namespace core {
inline namespace simple {

	/* Pause instruction to prevent excess processor bus usage */
	#define cpu_relax() asm volatile("pause\n": : :"memory")

	class Waiter {
		int i;
	public:
		Waiter() : i(0) {}

		void operator()() {
			++i;
			if ((i % 1000) == 0) {
				// there was no progress => let others work
				std::this_thread::yield();
			} else {
				// relax this CPU
				cpu_relax();
			}
		}
	};



    class SpinLock {
        std::atomic<int> lck;
    public:

        SpinLock() : lck(0) {
        }

        void lock() {
            Waiter wait;
            while(!try_lock()) wait();
        }

        bool try_lock() {
            int should = 0;
            return lck.compare_exchange_weak(should, 1, std::memory_order_acquire);
        }

        void unlock() {
            lck.store(0, std::memory_order_release);
        }
    };

} // end namespace simple
} // end namespace core
} // end namespace api
} // end namespace allscale
