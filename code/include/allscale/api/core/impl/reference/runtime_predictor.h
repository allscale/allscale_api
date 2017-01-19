#pragma once

#include <cstdint>
#include <cmath>
#include <thread>
#include <chrono>
#include <ostream>

namespace allscale {
namespace api {
namespace core {
namespace impl {
namespace reference {

	/**
	 * A utility to estimate the execution time of tasks on different
	 * levels of task-decomposition steps.
	 */
	class RuntimePredictor {

	public:

		using clock = std::chrono::high_resolution_clock;

		using duration = clock::duration;

		enum { MAX_LEVELS = 100 };

	private:

		/**
		 * The number of samples recorded per task level.
		 */
		std::array<std::size_t,MAX_LEVELS> samples;

		/**
		 * The current estimates of execution times of tasks.
		 */
		std::array<duration,MAX_LEVELS> times;

	public:

		RuntimePredictor(unsigned numWorkers = std::thread::hardware_concurrency()) {
			// reset number of collected samples
			samples.fill(0);

			// initialize time estimates
			times.fill(duration::zero());

			// initialize execution times up to a given level
			for(int i=0; i<std::log2(numWorkers) + 4; ++i) {
				times[i] = duration::max();
			}
		}

		/**
		 * Obtain a prediction of a given level.
		 */
		duration predictTime(std::size_t level) const {
			if (level >= MAX_LEVELS) return duration::zero();
			return times[level];
		}

		/**
		 * Update the predictions for a level.
		 */
		void registerTime(std::size_t level, const duration& time) {

			// update matching level
			updateTime(level,time);

			// update higher levels (with reduced weight)
			auto smallerTime = time / 2;
			auto largerTime = time * 2;
			for(std::size_t d = 1; d < 5; d++) {

				// update higher element
				if (d <= level) {
					updateTime(level-d,largerTime);
				}

				// update smaller element
				if (level+d < MAX_LEVELS) {
					updateTime(level+d,smallerTime);
				}

				// update parameters
				smallerTime = smallerTime / 2;
				largerTime = largerTime * 2;
			}

		}

		/**
		 * Enable the printing of the predictor state.
		 */
		friend std::ostream& operator<<(std::ostream& out, const RuntimePredictor& pred) {
			out << "Predictions:\n";
			for(int i = 0; i<MAX_LEVELS; i++) {
				auto us = std::chrono::duration_cast<std::chrono::microseconds>(pred.times[i]).count();
				out << "\t" << i << ": " << us << "us\n";
				if (us == 0) return out;
			}
			return out;
		}

	private:

		void updateTime(std::size_t level, const duration& time) {

			// update estimate of time of a task on this level
			auto N = samples[level];
			times[level] = (N * times[level] + time) / (N+1);

			// update sample count
			++samples[level];
		}

	};


} // end namespace reference
} // end namespace impl
} // end namespace core
} // end namespace api
} // end namespace allscale
