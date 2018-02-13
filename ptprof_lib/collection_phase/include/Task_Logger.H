#ifndef TASK_LOGGER_H
#define TASK_LOGGER_H

#include <unordered_map>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>

#include "Common.H"

#define NUM_ENTRIES 1024

struct step_work_data {
  size_t step_parent;
  size_t work;
  size_t node_id;
  unsigned short int node_type;
  ADDRINT ret_addr;
  std::unordered_map<size_t, size_t>* region_work;
};

class Task_Logger {
private:
  std::ofstream report[NUM_THREADS];
  unsigned int last_allocated[NUM_THREADS];
  struct step_work_data step_work_list[NUM_THREADS][NUM_ENTRIES] = {{{0}}};

public:
  Task_Logger();
  void log(THREADID,size_t,size_t,size_t,unsigned short int,ADDRINT);
  void log_intermediate_step(THREADID,size_t,size_t,size_t,unsigned short int,ADDRINT);
  void buffer_info(THREADID threadid,size_t creator_tid,size_t parent,size_t work,int node_type,ADDRINT ret_addr);
  void log_region_work(THREADID threadid,ADDRINT return_address,size_t work);
  void Fini(size_t creator_tid,size_t parent,size_t work,unsigned short int node_type,ADDRINT ret_addr);
  void log_sched_ov(THREADID threadid, size_t node_id, size_t work);  
};

#endif
