#ifndef UNIQUETASKID_H
#define UNIQUETASKID_H

#include <string>

struct unique_task_id {
  std::string seq_num_str;
  // unsigned int sum_seq_nos; // sum of sequence numbers of all nodes in the path from ...
  //                           // current node to root node - X-axis identifier
  unsigned int depth; // Depth used as Y-axis identifier to uniquely identify node in DPST
};

#endif
