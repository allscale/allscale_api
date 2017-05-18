#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "allscale/api/core/impl/reference/profiling.h"

using namespace allscale::api::core::impl::reference;

/**
 * A utility to convert collected performance data into graphical visualizations
 * of the resource efficiency of the application.
 */

using time_type = std::uint64_t;

bool exists(const std::string& file) {
	std::ifstream in(file.c_str());
	return in.good();
}

struct EventCounters {
	time_type timestamp = 0;
	int numTasksStarted = 0;
	int numTasksStolen = 0;
	int maxTaskDepth = 0;

	EventCounters& operator+=(const EventCounters& other) {
		numTasksStarted += other.numTasksStarted;
		numTasksStolen += other.numTasksStolen;
		maxTaskDepth = std::max(maxTaskDepth, other.maxTaskDepth);
		return *this;
	}

};


enum ActivityType {
	None,
	Task,
	Sleep,
	Steal
};

struct Activity {

	std::size_t thread;
	ActivityType activity;
	time_type begin;
	time_type end;

	bool operator<(const Activity& other) const {
		if (thread < other.thread) return true;
		if (thread > other.thread) return false;
		if (begin < other.begin) return true;
		if (begin > other.begin) return false;
		if (end < other.end) return true;
		if (end > other.end) return false;
		return activity < other.activity;
	}

};

struct AnalysisConfig {
	bool aggregateActivities = true;
	time_type startTime = 0;
	time_type duration = 0;
	time_type numSamples = 1200;
};

struct AnalysisResult {

	/**
	 * A list of counters for the line charts.
	 */
	std::vector<EventCounters> counters;

	/**
	 * The activities for the timeline chart.
	 */
	std::vector<Activity> activities;

};

/**
 * A function loading profiler log files from the current working directory.
 */
std::vector<ProfileLog> loadLogs();

/**
 * The analysis step, processing log files and extracting the analysis results to be
 * visualized in the end.
 */
AnalysisResult analyseLogs(const std::vector<ProfileLog>& logs, const AnalysisConfig& config);

/**
 * The operation creating the resulting html report.
 */
void createReport(const AnalysisResult& result, const AnalysisConfig&);

/**
 * Prints the usage of this program.
 */
void printUsageAndExit(const std::string& name) {
	std::cout << "Usage: " << name << " [options]\n";
	std::cout << "  Options:\n";
	std::cout << "  \t--no-aggregate      disable task aggregation\n";
	std::cout << "  \t--start <num>       specify lower start time limit in ms\n";
	std::cout << "  \t--duration <num>    specify upper end time limit in ms\n";
	std::cout << "  \t--samples <num>     specify number of samples to take for aggregation\n";
	std::cout << "  \t--help,-h           display this help text\n";
	exit(0);
}

/**
 * The main entry point, conducting the necessary analysis steps.
 */
int main(int argc, char** argv) {

	AnalysisConfig config;
	config.aggregateActivities = true;

	// parse parameters
	for(int i=1; i<argc; i++) {
		std::string flag = argv[i];
		if (flag == "--no-aggregate") {
			config.aggregateActivities = false;
		}
		if(flag == "-h" || flag == "--help") {
			printUsageAndExit(argv[0]);
		}
		if (flag == "--start") {
			i++;
			if(argc <= i) {
				printUsageAndExit(argv[0]);
			}
			config.startTime = std::atol(argv[i]);
		}
		if(flag == "--duration") {
			i++;
			if(argc <= i) {
				printUsageAndExit(argv[0]);
			}
			config.duration = std::atol(argv[i]);
		}
		if(flag == "--samples") {
			i++;
			if(argc <= i) {
				printUsageAndExit(argv[0]);
			}
			config.numSamples = std::atoi(argv[i]);
		}
	}


	// print welcome note
	std::cout << "--- AllScale API Reference Implementation Profiling Tool (beta) ---\n";
	std::cout << "Loading logs ...\n";

	auto logs = loadLogs();


	// extract event data
	std::cout << "Analysing data ...\n";
	auto res = analyseLogs(logs,config);

	// produce html report
	std::cout << "Producing report ...\n";
	createReport(res,config);

	// open the report in the browser
	return system("xdg-open report.html > /dev/null");
}

// -------------------------------------------------------------------
// 							Implementations
// -------------------------------------------------------------------

std::vector<ProfileLog> loadLogs() {

	// load all available logs
	std::vector<ProfileLog> logs;

	// loading input files
	int i = 0;
	std::string file = getLogFileNameForWorker(i);
	while(exists(file)) {
		// load this file
		std::cout << "  loading file " << file << " ...\n";
		logs.emplace_back(ProfileLog::loadFrom(file));

		// go to next step
		i++;
		file = getLogFileNameForWorker(i);
	}

	return logs;
}

std::vector<EventCounters> aggregateEventCounters(const std::vector<EventCounters>& events, const AnalysisConfig& config) {
	std::vector<EventCounters> res;

	const double factor = events.size() / (double)config.numSamples;
	for(std::size_t i = 0; i < config.numSamples; ++i) {
		EventCounters temp = events[(time_type)(i*factor)];
		for(std::size_t j = std::size_t(i*factor) + 1; j < (i + 1)*factor; ++j) {
			temp += events[j];
		}
		res.push_back(temp);
	}

	return res;
}

std::vector<EventCounters> extractEventCounters(const std::vector<ProfileLog>& logs, const AnalysisConfig& config) {

	std::cout << "  extracting event counts ...\n";

	// compute time limits
	auto mintime = std::numeric_limits<time_type>::max();
	auto maxtime = std::numeric_limits<time_type>::min();
	for(const auto& cur : logs) {
		for(const auto& event : cur) {
			mintime = std::min(event.getTimestamp(),mintime);
			maxtime = std::max(event.getTimestamp(),maxtime);
		}
	}

	// a utility to shift time stamps to base 0
	auto shift = [&](time_type time){ return (time - mintime) / 1000000; };

	// adjust max-time to ms
	auto num_timesteps = shift(maxtime) + 1;
	// user-specified duration overrides
	if(config.duration > 0) {
		num_timesteps = config.duration + 1;
	}

	// initialize the event counter
	auto res = std::vector<EventCounters>(num_timesteps);

	// initialize time stamps
	for(std::size_t i = 0; i < res.size(); ++i) {
		res[i].timestamp = config.startTime + i;
	}

	for(const auto& cur : logs) {
		for(const auto& event : cur) {

			auto time = shift(event.getTimestamp());
			// if timestamp is outside user-specified limits, ignore
			if(time < config.startTime || time > config.startTime + num_timesteps) continue;
			// shift timestamp base to 0
			auto index = time - config.startTime;

			switch(event.getKind()) {
			case ProfileLogEntry::TaskStarted:
				res[index].numTasksStarted++;
				res[index].maxTaskDepth = std::max<int>(res[index].maxTaskDepth,event.getTask().getDepth());
				break;
			case ProfileLogEntry::TaskStolen:  res[index].numTasksStolen++;  break;
			default: break;
			}
		}
	}

	if(config.aggregateActivities) {
		res = aggregateEventCounters(res,config);
	}

	// done
	return res;
}

std::vector<Activity> aggregateActivities(const std::vector<Activity>& actions, const AnalysisConfig& config, time_type maxtime, std::size_t numWorker) {
	// create empty masks
	std::vector<std::vector<ActivityType>> masks;

	const time_type length = std::min<time_type>(maxtime, config.numSamples);
	const double factor = maxtime/(double)length;

	// initialize with inactive tasks
	for(std::size_t i=0; i<numWorker; ++i) {
		masks.emplace_back(length+2,None);
	}

	// fill in tasks
	for(const auto& cur : actions) {
		auto& mask = masks[cur.thread];
		for(time_type i = cur.begin; i < cur.end; ++i) {
			assert_le(i, maxtime);
			std::size_t pos = (std::size_t)(i / factor);
			mask[pos] = std::max(mask[pos],cur.activity);
		}
	}

	// extract tasks
	std::vector<Activity> res;
	for(std::size_t t=0; t<numWorker; ++t) {

		const auto& mask = masks[t];

		time_type begin = 0;
		ActivityType last = None;
		for(std::size_t i=0; i<=(length+1); ++i) {
			// if nothing changed, we are fine
			if (last == mask[i]) continue;

			// add a new activity
			res.push_back(Activity{
				t,last,begin,(time_type)(i*factor)
			});

			// start a new phase
			last = mask[i];
			begin = (time_type)(i*factor);
		}
	}

	// return aggregated data
	return res;
}

std::vector<Activity> extractActivities(const std::vector<ProfileLog>& logs, const AnalysisConfig& config) {

	std::cout << "  extracting activities ...\n";

	time_type startTime = std::numeric_limits<time_type>::max();
	for(const auto& cur : logs) {
		startTime = std::min(startTime,(*cur.begin()).getTimestamp());
	}

	// extract start and end times of all work items
	std::map<TaskID,std::size_t> thread;
	std::map<TaskID,std::size_t> start;
	std::map<TaskID,std::size_t> end;

	// create activity list
	std::vector<Activity> res;

	time_type sleeptime = config.startTime;
	time_type maxtime = 0;

	// analyse data
	for(std::size_t i=0; i<logs.size(); ++i) {
		auto& log = logs[i];

		for(const auto& entry : log) {

			// process the timestamp (convert to ms)
			const auto timestamp = (entry.getTimestamp() - startTime) / 1000000;
			// if timestamp is outside user-specified limits, ignore
			if(timestamp < config.startTime || (timestamp > config.startTime + config.duration && config.duration > 0)) continue;
			
			// remember the thread
			thread[entry.getTask()] = i;

			// keep track of the maxtime
			maxtime = std::max(timestamp,maxtime);

			// record start and end times
			switch(entry.getKind()) {

			case ProfileLogEntry::TaskStolen:
				res.push_back(Activity{
					i, ActivityType::Steal, timestamp, timestamp + (config.aggregateActivities ? 1 : 0)
				});
				break;

			case ProfileLogEntry::TaskStarted:
				start[entry.getTask()] = timestamp;
				break;

			case ProfileLogEntry::TaskEnded:
				end[entry.getTask()] = timestamp;
				break;

			case ProfileLogEntry::WorkerSuspended:
				sleeptime = timestamp;
				break;

			case ProfileLogEntry::WorkerResumed:
				res.push_back(Activity{
					i, ActivityType::Sleep, sleeptime, timestamp
				});
				break;

			default: break;
			}
		}
	}

	// produce task list
	for(const auto& cur : thread) {
		// TODO: fix task maps so this check is not necessary?
		if(start.find(cur.first) == start.end() || end.find(cur.first) == end.end()) { continue; }
		res.push_back(Activity{
			cur.second,
			ActivityType::Task,
			start.at(cur.first),
			end.at(cur.first)
		});
	}

	// sort events
	std::sort(res.begin(),res.end());

	// aggregate if requested
	if (config.aggregateActivities) {
		res = aggregateActivities(res,config,maxtime,logs.size());
	}
	
	// done
	return res;
}

AnalysisResult analyseLogs(const std::vector<ProfileLog>& logs, const AnalysisConfig& config) {

	return AnalysisResult {
		extractEventCounters(logs,config),
		extractActivities(logs,config)
	};

}


void createReport(const AnalysisResult& result, const AnalysisConfig& config) {

	std::ofstream out("report.html");

	// print header
	out << R"(
		<html>
		  <head>
			<script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
			<script type="text/javascript">
			  google.charts.load('current', {'packages':['corechart','timeline']});
			  google.charts.setOnLoadCallback(drawChart);

			  function drawLineChart() {
				var data = google.visualization.arrayToDataTable([
	)";

	// print chart data
	out << "['time','tasks started','tasks stolen','max_task_depth'],\n";

	for(std::size_t t = 0; t<result.counters.size(); t++) {
		out << "["
				<< result.counters[t].timestamp/1000.0 << ","
				<< result.counters[t].numTasksStarted << ","
				<< result.counters[t].numTasksStolen << ","
				<< result.counters[t].maxTaskDepth
			<< "],\n";
	}

	out << R"(]);
		
				var options = {
				  title: 'Program Event Counts',
				  legend: { position: 'bottom' },
				  chartArea:{left:45,top:20,width:'98%',height:'80%'}
				};
		
				var chart = new google.visualization.LineChart(document.getElementById('taskcreation'));
		
				chart.draw(data, options);
			  }

			  function drawTimelineChart() {
				var container = document.getElementById('timeline');
				var chart = new google.visualization.Timeline(container);
				var dataTable = new google.visualization.DataTable();
		
				dataTable.addColumn({ type: 'string', id: 'Thread' });
				dataTable.addColumn({ type: 'string', id: 'Action' });
				dataTable.addColumn({ type: 'string', role: 'style' });
				dataTable.addColumn({ type: 'number', id: 'Start' });
				dataTable.addColumn({ type: 'number', id: 'End' });
				dataTable.addRows([
	)";

	struct Style {
		std::string label;
		std::string color;
	};

	std::map<ActivityType, Style> appearance = { { None,	{ "unknown",	"#FFFFFF" } },
												 { Task,	{ "task",		"#3366CC" } },
												 { Steal,	{ "steal",		"#DC3912" } },
												 { Sleep,	{ "sleep",		"#FF9900" } } };
	// print initial entry for compatibility
	out << "[ 'T0', '" << appearance[ActivityType::None].label << "', '" << appearance[ActivityType::None].color << "', " << config.startTime << ", " << config.startTime << "],\n";


	// print timeline data
	for(const auto& cur : result.activities) {

		if(cur.activity == None) { continue; }

		out << "[ 'T" << cur.thread << "', '" << appearance[cur.activity].label << "', '" << appearance[cur.activity].color << "', " << cur.begin << ", " << cur.end << "],\n";
	}

	// print tail
	out << R"(
				]);

				var options = {
				  timeline: { showBarLabels: false },
				};
		
				chart.draw(dataTable, options);
			  }
		
			  function drawChart() {
				  drawLineChart();
				  drawTimelineChart();
			  }
			</script>
		  </head>
		  <body>
			<div id="taskcreation" style="height: 200px;"></div>
			<div id="timeline" style="height: 30000px;"></div>
		  </body>
		</html>
	)";


	// done

}
