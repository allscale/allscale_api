#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>

#include "allscale/api/core/impl/reference/profiling.h"

using namespace allscale::api::core::impl::reference;

/**
 * A utility to convert collected performance data into graphical visualizations
 * of the resource efficiency of the application.
 */


bool exists(const std::string& file) {
	std::ifstream in(file.c_str());
	return in.good();
}

struct EventCounters {
	int numTasksStarted = 0;
	int numTasksStolen = 0;
	int maxTaskDepth = 0;
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
	std::uint64_t begin;
	std::uint64_t end;

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
void createReport(const AnalysisResult& result);

/**
 * The main entry point, conducting the necessary analysis steps.
 */
int main() {

	AnalysisConfig config;
	config.aggregateActivities = true;

	// print welcome note
	std::cout << "--- AllScale API Reference Implementation Profiling Tool (beta) ---\n";
	auto logs = loadLogs();


	// extract event data
	std::cout << "Analysing data ...\n";
	auto res = analyseLogs(logs,config);

	// produce html report
	std::cout << "Producing report ...\n";
	createReport(res);

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

std::vector<EventCounters> extractEventCounters(const std::vector<ProfileLog>& logs, const AnalysisConfig&) {

	// compute time limits
	auto mintime = std::numeric_limits<std::uint64_t>::max();
	auto maxtime = std::numeric_limits<std::uint64_t>::min();;
	for(const auto& cur : logs) {
		for(const auto& event : cur) {
			mintime = std::min(event.getTimestamp(),mintime);
			maxtime = std::max(event.getTimestamp(),maxtime);
		}
	}

	// a utility to normalize time stamps
	auto normalize = [&](std::uint64_t time){ return (time - mintime) / 1000000; };

	// adjust max-time to ms
	auto num_timesteps = normalize(maxtime) + 1;

	// initialize the event counter
	auto res = std::vector<EventCounters>(num_timesteps);

	for(const auto& cur : logs) {
		for(const auto& event : cur) {

			auto time = normalize(event.getTimestamp());

			switch(event.getKind()) {
			case ProfileLogEntry::TaskStarted:
				res[time].numTasksStarted++;
				res[time].maxTaskDepth = std::max<std::size_t>(res[time].maxTaskDepth,event.getTask().getDepth());
				break;
			case ProfileLogEntry::TaskStolen:  res[time].numTasksStolen++;  break;
			default: break;
			}
		}
	}

	// done
	return res;
}


std::vector<Activity> aggregateActivities(const std::vector<Activity>& actions, std::uint64_t maxtime, std::size_t numWorker) {

	// create empty masks
	std::vector<std::vector<ActivityType>> masks;

	// initialize with inactive tasks
	for(std::size_t i=0; i<numWorker; ++i) {
		masks.emplace_back(maxtime+2,None);
	}

	// fill in tasks
	for(const auto& cur : actions) {
		auto& mask = masks[cur.thread];
		for(std::uint64_t i = cur.begin; i < cur.end; ++i) {
			mask[i] = std::max(mask[i],cur.activity);
		}
	}

	// extract tasks
	std::vector<Activity> res;
	for(std::size_t t=0; t<numWorker; ++t) {

		auto& mask = masks[t];

		std::uint64_t begin = 0;
		ActivityType last = None;
		for(std::size_t i=0; i<=maxtime+1; ++i) {
			// if nothing changed, we are fine
			if (last == mask[i]) continue;

			// add a new activity
			res.push_back(Activity{
				t,last,begin,i
			});

			// start a new phase
			last = mask[i];
			begin = i;
		}
	}

	// return aggregated data
	return res;
}

std::vector<Activity> extractActivities(const std::vector<ProfileLog>& logs, const AnalysisConfig& config) {

	std::uint64_t startTime = std::numeric_limits<std::uint64_t>::max();
	for(const auto& cur : logs) {
		startTime = std::min(startTime,(*cur.begin()).getTimestamp());
	}

	// extract start and end times of all work items
	std::map<TaskID,std::size_t> thread;
	std::map<TaskID,std::size_t> start;
	std::map<TaskID,std::size_t> end;

	// create activity list
	std::vector<Activity> res;

	std::uint64_t sleeptime = 0;
	std::uint64_t maxtime = 0;

	// analyse data
	for(std::size_t i=0; i<logs.size(); ++i) {
		auto& log = logs[i];

		for(const auto& entry : log) {

			// remember the thread
			thread[entry.getTask()] = i;

			// process the timestamp (convert to ms)
			auto timestamp = (entry.getTimestamp() - startTime) / 1000000;

			// keep track of the maxtime
			maxtime = std::max(timestamp,maxtime);

			// record start and end times
			switch(entry.getKind()) {

			case ProfileLogEntry::TaskStolen:
				res.push_back(Activity{
					i, ActivityType::Steal, timestamp, timestamp + 1
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
		res.push_back(Activity{
			cur.second,
			ActivityType::Task,
			start[cur.first],
			end[cur.first]
		});
	}

	// sort events
	std::sort(res.begin(),res.end());

	// aggregate if requested
	if (config.aggregateActivities) {
		res = aggregateActivities(res,maxtime,logs.size());
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


void createReport(const AnalysisResult& result) {

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
				<< (t/1000.0) << ","
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
				dataTable.addColumn({ type: 'number', id: 'Start' });
				dataTable.addColumn({ type: 'number', id: 'End' });
				dataTable.addRows([
					[ 'T0', 'task', 0, 0 ],
	)";


	// print timeline data
	for(const auto& cur : result.activities) {

		const char* info = "unknown";
		switch(cur.activity) {
		case None:  continue;
		case Task:  info = "task"; break;
		case Steal: info = "steal"; break;
		case Sleep: info = "sleep"; break;
		}

		out << "[ 'T" << cur.thread << "', '" << info << "', " << cur.begin << ", " << cur.end << "],\n";
	}


	// print tail
	out << R"(
				]);
		
				var options = {
				  timeline: { showBarLabels: false } 
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
