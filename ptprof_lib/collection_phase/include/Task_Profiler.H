#ifndef TASK_PROFILER_H
#define TASK_PROFILER_H

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <fstream>
#include <vector>
#include <string>
#include <map>

#include "AFTaskGraph.H"
#include "Common.H"

#define NUM_ENTRIES 1024

extern AFTaskGraph* taskGraph;

struct step_work_data {
  //char* seq_num_str;
  //unsigned int sum_seq_nos;
  //unsigned int depth;
  size_t step_parent;
  size_t work;
  std::map<size_t, size_t>* region_work;
};

class Task_Profiler {
private:
  //PIN_LOCK prof_lock;
  std::ofstream report[NUM_THREADS];

  unsigned int last_allocated[NUM_THREADS];
  struct step_work_data step_work_list[NUM_THREADS][NUM_ENTRIES] = {{{0}}};
  //std::vector<struct step_work_data> step_work_list[NUM_THREADS];
  std::map<size_t, struct CallSiteData*> regionMap;

  int perf_fds[NUM_THREADS];
  int perf_event_open_wrapper(struct perf_event_attr *hw_event, pid_t pid,
			      int cpu, int group_fd, unsigned long flags);
  size_t stop_n_get_count (THREADID threadid);
  void start_count(THREADID threadid);
  
public:
  Task_Profiler();
  void TP_CaptureExecute(THREADID threadid);
  void TP_CaptureReturn(THREADID threadid);
  void TP_CaptureWait_Entry(THREADID threadid);
  void TP_CaptureWait_Exit(THREADID threadid);
  void TP_CaptureSpawn_Entry(THREADID threadid);
  void TP_CaptureSpawn_Exit(THREADID threadid);
  void TP_CaptureBeginOptimize(THREADID threadid, const char* file, int line, void* return_address);
  void TP_CaptureEndOptimize(THREADID threadid, const char* file, int line, void* return_address);
  void Fini();
  void dumpRegionInfo();
};

#endif
