#ifndef AFTASK_GRAPH_H
#define AFTASK_GRAPH_H

#include <stack>
#include <map>
#include <fstream>

#include "AFTask.H"
#include "Common.H"
#include "CallSiteData.H"

class AFTaskGraph {
protected:
  std::ofstream report_int_nodes;

  PIN_LOCK lock;
  std::stack<struct AFTask*> cur_dpst_index[NUM_THREADS];
  std::map<size_t, struct AFTask*> temp_cur_map;
  std::map<size_t, struct CallSiteData*> callSiteMap;
  size_t last_allocated_node;

  void CaptureSpawnOnly(THREADID threadid, size_t taskId, void* return_address);
  void CaptureSpawnAndWait(THREADID threadid, size_t taskId, void* return_address);
  struct AFTask* create_node();

public:
  AFTaskGraph();
  void CaptureExecute(THREADID threadid, size_t taskId);
  void CaptureReturn(THREADID threadid);
  void CaptureWaitOnly(THREADID threadid);
  void CaptureWait(THREADID threadid);
  void CaptureSetTaskId(THREADID threadid, size_t taskId, 
			int sp_only, void* return_address, 
			const char* file, int line, bool par_for = false);
  size_t getCurStep(THREADID threadid);
  //struct CallSiteData* getSourceFileAndLine(size_t return_address);
  void dumpCallsiteInfo();
};

#endif
