#ifndef AFTASK_GRAPH_H
#define AFTASK_GRAPH_H

#include <stack>
#include <map>
#include <sys/mman.h>

#include "AFTask.H"
#include "Common.H"
#include "CallSiteData.H"

#define MMAP_FLAGS (MAP_PRIVATE| MAP_ANONYMOUS| MAP_NORESERVE)
#define NUM_GRAPH_NODES 134217728

class AFTaskGraph {
protected:
  std::stack<size_t> cur_dpst_index[NUM_THREADS];
  std::map<size_t, size_t> temp_cur_map;

  std::map<size_t, struct CallSiteData*> callSiteMap;

  void CaptureSpawnOnly(THREADID threadid, size_t taskId, void* return_address);
  void CaptureSpawnAndWait(THREADID threadid, size_t taskId, void* return_address);
  bool checkForStep (struct AFTask* cur_node);
  size_t create_node(NodeType node_type, size_t task_id);
  void initialize_task (size_t index,NodeType node_type, size_t val);

public:
  size_t last_allocated_node;
  struct AFTask* tgraph_nodes;

  AFTaskGraph();
  void CaptureExecute(THREADID threadid, size_t taskId);
  void CaptureReturn(THREADID threadid);
  void CaptureWaitOnly(THREADID threadid);
  void CaptureWait(THREADID threadid);
  void CaptureSetTaskId(THREADID threadid, size_t taskId, 
			int sp_only, void* return_address, 
			const char* file, int line, bool par_for = false);

  struct AFTask* getCurTask(THREADID threadid);
  struct AFTask* getCurParent(THREADID threadid);
  struct AFTask* getCurStep(THREADID threadid);
  struct AFTask* getHead() {return &tgraph_nodes[1];}
  struct AFTask* getTask(size_t index) {return &tgraph_nodes[index];}
  struct CallSiteData* getSourceFileAndLine(size_t return_address);
};

#endif
