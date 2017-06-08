#ifndef AFTASK_H
#define AFTASK_H

#include "unique_task_id.H"

enum NodeType {
  NO_TYPE = 0,
  ASYNC = 1,
  FINISH = 2,
  STEP = 3
};

struct AFTask {
  //size_t taskId;
  //size_t cur_step;
  //NodeType type;
  //size_t parent;
  //unsigned int num_children;
  //struct unique_task_id ut_id;

  NodeType young_ns_child; //NodeType of youngest non-step child  
  bool sync_finish_flag;

  size_t node_id;

  /* profile data for each task */
  //struct profiler_data t_prof;
  //size_t call_site;
};

#endif
