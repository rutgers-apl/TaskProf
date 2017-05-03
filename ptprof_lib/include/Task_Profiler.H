#ifndef TASK_PROFILER_H
#define TASK_PROFILER_H

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <fstream>

#include "AFTaskGraph.H"
#include "WorkSpanData.H"
#include "CallSiteData.H"

extern AFTaskGraph* taskGraph;

class Task_Profiler {
private:
  std::ofstream report;

  int perf_fds[NUM_THREADS];
  int perf_event_open_wrapper(struct perf_event_attr *hw_event, pid_t pid,
			      int cpu, int group_fd, unsigned long flags);
  //regionmap may not be thread-safe. But inserting the same data, to the same key
  // by two parallel threads is ok. Maybe taking a risk
  std::map<size_t, struct CallSiteData*> regionMap;

  size_t stop_n_get_count (THREADID threadid);
  void start_count(THREADID threadid);
  bool recursiveCall(struct AFTask* node);
  void checkUpdateParentWorkSpan(size_t parent_index, size_t* num_processed, std::map<size_t, struct WorkSpanData>* workSpanMap);
  void calculateRecurse(struct AFTask* node, std::map<size_t, struct WorkSpanData>* workSpanMap);
  void calculateWorkSpan(struct AFTask* node);

  /********** INCREASE PARALLELISM OF EACH CALLSITE **************/
  void report_cs_increase_par(std::map<size_t,struct WorkSpanData> workSpanMap);
  void update_critical_path(size_t ret_addr, unsigned int par_increase);

  void checkUpdateParentWorkSpanRepeat(size_t parent_index, size_t* num_processed, size_t updated_cs);
  void calculateRecurseRepeat(struct AFTask* node, size_t updated_cs);
  void calculateWorkSpanRepeat(struct AFTask* node, size_t updated_cs);
  /***************************************************************/

  /********** INCREASE PARALLELISM OF EACH STATIC REGION **************/
  void report_region_increase_par();
  void checkUpdateParentWorkSpan_region(size_t parent_index, size_t* num_processed, size_t updated_region, unsigned int par_increase);
  void calculateRecurse_region(struct AFTask* node, size_t updated_region, unsigned int par_increase);
  void calculateWorkSpan_region(struct AFTask* node, size_t updated_region, unsigned int par_increase);
  void reset_work_span();
  /***************************************************************/
  
  
  /********** REPORT %AGE OF CRITICAL PATH LENGTH **************/
  void merge_critical_call_sites(std::map<size_t, size_t>*, std::map<size_t, size_t>*);
  void insert_cs_data(std::map<size_t, size_t>*, size_t call_site, size_t local_local_work);
  /***************************************************************/

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
};

#endif
