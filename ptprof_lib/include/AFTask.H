#ifndef AFTASK_H
#define AFTASK_H

#include <map>

enum NodeType {
  NO_TYPE = 0,
  ASYNC = 1,
  FINISH = 2,
  STEP = 3
};

struct profiler_data {
  size_t work;
  size_t local_work;
  size_t critical_child;
  size_t parent_work;

  /**** CALCULATING %AGE OF CRITICAL PATH AND REGION ****/
  size_t local_local_work;
  std::map<size_t, size_t>* critical_call_sites;

  std::map<size_t, size_t>* region_work;
  /**** CALCULATING %AGE OF CRITICAL PATH REGION ****/
};

struct AFTask {
  size_t taskId;
  size_t parent;
  size_t cur_step;
  NodeType type;
  size_t num_children;
  NodeType young_ns_child; //NodeType of youngest non-step child
  
  bool sp_root_n_wt_flag;

  /* profile data for each task */
  struct profiler_data t_prof;
  size_t call_site;
};

#endif
