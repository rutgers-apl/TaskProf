#ifndef AFTASK_H
#define AFTASK_H

#include "unique_task_id.H"
#include "Location.H"

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
  std::unordered_map<size_t, size_t>* critical_call_sites;

  std::unordered_map<size_t, size_t>* region_work;
  /**** CALCULATING %AGE OF CRITICAL PATH REGION ****/
};

struct AFTask {
  //size_t taskId;
  //size_t cur_step;
  //NodeType young_ns_child; //NodeType of youngest non-step child  
  bool sp_root_n_wt_flag;
  //struct unique_task_id ut_id;

  size_t parent;
  NodeType type;
  size_t num_children;

  /* profile data for each task */
  struct profiler_data t_prof;
  size_t call_site;

  struct location start;
  struct location end;

  size_t sc_ov_work = 0;
  
  /***** AUTOMATIC REGION DETECTION **********/
  AFTask* local_step;
  AFTask* critical_step;
  AFTask* parent_step;
  /***** AUTOMATIC REGION DETECTION **********/
};

#endif
