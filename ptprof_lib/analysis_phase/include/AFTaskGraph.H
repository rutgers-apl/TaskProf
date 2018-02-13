#ifndef AFTASK_GRAPH_H
#define AFTASK_GRAPH_H

#include <string>
#include <unordered_map>
#include <sys/mman.h>
#include <sstream>
#include <fstream>
#include <vector>

#include "AFTask.H"
#include "Common.H"
#include "CallSiteData.H"

#define MMAP_FLAGS (MAP_PRIVATE| MAP_ANONYMOUS| MAP_NORESERVE)
//#define NUM_GRAPH_NODES 134217728
#define NUM_GRAPH_NODES 146650394//146641882//146638554

class AFTaskGraph {
protected:
  std::unordered_map<size_t, struct CallSiteData*> callSiteMap;
  std::unordered_map<std::string, std::vector<struct CallSiteData> > par_step_work_info;
  std::unordered_map<std::string, std::vector<struct CallSiteData> > ser_step_work_info;

  void initialize_task (size_t index,NodeType node_type, size_t call_site, size_t parent_idx);
  void create_dpst(std::string, bool);
  void print_dpst();

public:
  size_t last_allocated_node;
  struct AFTask* tgraph_nodes;

  AFTaskGraph(std::string);
  struct AFTask* getHead() {return &tgraph_nodes[1];}
  struct AFTask* getTask(size_t index) {return &tgraph_nodes[index];}
  struct CallSiteData* getSourceFileAndLine(size_t return_address, std::string folder);
  void initCallSiteMap(std::string folder);
};

#endif
