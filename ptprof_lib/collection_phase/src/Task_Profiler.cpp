#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
//#include <assert.h>
#include "unique_task_id.H"
#include "Task_Profiler.H"

//#include "CallSiteData.H"

Task_Profiler::Task_Profiler() {
  start_count(0);
}

int Task_Profiler::perf_event_open_wrapper(struct perf_event_attr *hw_event, pid_t pid,
		int cpu, int group_fd, unsigned long flags)
{
  int ret;
  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
		group_fd, flags);
  return ret;
}

void Task_Profiler::start_count(THREADID threadid) {
#if 1
  struct perf_event_attr pe;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_HARDWARE;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = PERF_COUNT_HW_CPU_CYCLES;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.exclude_idle = 1;

  int fd;
  fd = perf_event_open_wrapper(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    fprintf(stderr, "Unable to read performance counters. Linux perf event API not supported on the machine. Error number: %d\n", errno);
    exit(EXIT_FAILURE);
  }
  perf_fds[threadid] = fd;

  ioctl(fd, PERF_EVENT_IOC_RESET);
  ioctl(fd, PERF_EVENT_IOC_ENABLE);
#endif
}

size_t Task_Profiler::stop_n_get_count (THREADID threadid) {
#if 1
  ioctl(perf_fds[threadid], PERF_EVENT_IOC_DISABLE);
  size_t count = 0;
  read(perf_fds[threadid], &count, sizeof(unsigned long));
  close(perf_fds[threadid]);
  return count;
#else
  return 1;
#endif
}

void Task_Profiler::update_step_and_region(THREADID threadid) {
  taskLogger->update_step_and_region(threadid,taskGraph->getCurStepRegionBegin(threadid),taskGraph->getCurStepRegionEnd(threadid));
}

void Task_Profiler::TP_CaptureExecute(THREADID threadid) {  
  start_count(threadid);
}

void Task_Profiler::TP_CaptureReturn(THREADID threadid) {
  size_t count = stop_n_get_count(threadid);
  taskLogger->log(threadid,taskGraph->getCurStepId(threadid),taskGraph->getCurStepParent(threadid),count,STEP,0);
}

void Task_Profiler::TP_CaptureWait_Entry(THREADID threadid) {
  size_t count = stop_n_get_count(threadid);
  taskLogger->log(threadid,taskGraph->getCurStepId(threadid),taskGraph->getCurStepParent(threadid),count,STEP,0);  
}

void Task_Profiler::TP_CaptureWait_Exit(THREADID threadid) {
  start_count(threadid);
}

void Task_Profiler::TP_CaptureSpawn_Entry(THREADID threadid) {
  size_t count = stop_n_get_count(threadid);
  taskLogger->log(threadid,taskGraph->getCurStepId(threadid),taskGraph->getCurStepParent(threadid),count,STEP,0);
}

void Task_Profiler::TP_CaptureSpawn_Exit(THREADID threadid) {
  start_count(threadid);
}

void Task_Profiler::TP_CaptureBeginOptimize(THREADID threadid, const char* file, int line, void* return_address) {
  size_t count = stop_n_get_count(threadid);
  taskLogger->log(threadid,taskGraph->getCurStepId(threadid),taskGraph->getCurStepParent(threadid),count,STEP,0);
  taskLogger->update_file_line(threadid,taskGraph->getCurStepRegionBegin(threadid),taskGraph->getCurStepRegionEnd(threadid));
  start_count(threadid);
}

void Task_Profiler::TP_CaptureEndOptimize(THREADID threadid, const char* file, int line, void* return_address) {
  size_t count = stop_n_get_count(threadid);  
  taskLogger->buffer_info(threadid,taskGraph->getCurStepId(threadid),taskGraph->getCurStepParent(threadid),count,STEP,0);
  taskLogger->update_file_line(threadid,taskGraph->getCurStepRegionBegin(threadid),taskGraph->getCurStepRegionEnd(threadid));
  
  if (regionMap.count((ADDRINT)return_address) == 0) {
    struct CallSiteData* callsiteData = new CallSiteData();
    callsiteData->cs_filename = file;
    callsiteData->cs_line_number = line;
    regionMap.insert(std::pair<size_t, struct CallSiteData*>((ADDRINT)return_address, callsiteData));
  }

  taskLogger->log_region_work(threadid,(ADDRINT)return_address,count);
  start_count(threadid);
}

void Task_Profiler::Fini() {
  size_t count = stop_n_get_count(0);

  taskLogger->Fini(taskGraph->getCurStepId(0),
		   taskGraph->getCurStepParent(0),
		   count,
		   STEP,
		   0,
		   taskGraph->getCurStepRegionBegin(0),
		   taskGraph->getCurStepRegionEnd(0));


  //write callsite info to file
  taskGraph->dumpCallsiteInfo();

  if (!regionMap.empty()) {
    dumpRegionInfo();
  }
}

void Task_Profiler::dumpRegionInfo() {
  std::ofstream report_region_info;
  report_region_info.open("region_info.csv");
  for (std::unordered_map<size_t,struct CallSiteData*>::iterator it=regionMap.begin();
	 it!=regionMap.end(); ++it) {
    struct CallSiteData* cs_data = it->second;
    report_region_info << it->first << ","
		       << cs_data->cs_filename << ","
		       << cs_data->cs_line_number
		       << std::endl;
  }
  report_region_info.close();
}
