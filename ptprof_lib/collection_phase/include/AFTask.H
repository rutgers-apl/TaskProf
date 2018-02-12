#ifndef AFTASK_H
#define AFTASK_H

#include "Location.H"

enum NodeType {
  NO_TYPE = 0,
  ASYNC = 1,
  FINISH = 2,
  STEP = 3
};

struct AFTask {
  NodeType young_ns_child; //NodeType of youngest non-step child  
  bool sync_finish_flag;
  size_t node_id;
  size_t child_id;
  struct location start;
  struct location end;
};

#endif
