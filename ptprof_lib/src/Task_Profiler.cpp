//#include <assert.h>
#include <iostream>
#include <string.h>

#include "Task_Profiler.H"
#include "CallSiteData.H"

#define PAR_INC_COUNT 4
#define PAR_INC_INIT 50
#define PAR_INC_FACTOR 2

Task_Profiler::Task_Profiler() {
  start_count(0);
}

int Task_Profiler::perf_event_open_wrapper(struct perf_event_attr *hw_event, pid_t pid,
		int cpu, int group_fd, unsigned long flags)
{
  int ret;
  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
		group_fd, flags);
  return ret;
}

void Task_Profiler::start_count(THREADID threadid) {
#if 1
  struct perf_event_attr pe;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_HARDWARE;
  pe.size = sizeof(struct perf_event_attr);
  //pe.config = PERF_COUNT_HW_INSTRUCTIONS;
  pe.config = PERF_COUNT_HW_CPU_CYCLES;
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.exclude_idle = 1;

  int fd;
  fd = perf_event_open_wrapper(&pe, 0, -1, -1, 0);
  if (fd == -1) {
    std:: cout << "ERR NOS:" << EMFILE << std::endl;
    fprintf(stderr, "Error opening leader. Error number: %d\n", errno);
    exit(EXIT_FAILURE);
  }
  perf_fds[threadid] = fd;

  ioctl(fd, PERF_EVENT_IOC_RESET);
  ioctl(fd, PERF_EVENT_IOC_ENABLE);
#endif
}

size_t Task_Profiler::stop_n_get_count (THREADID threadid) {
#if 1
  ioctl(perf_fds[threadid], PERF_EVENT_IOC_DISABLE);
  size_t count = 0;
  read(perf_fds[threadid], &count, sizeof(unsigned long));
  close(perf_fds[threadid]);
  return count;
#else
  return 1;
#endif
}

void Task_Profiler::TP_CaptureExecute(THREADID threadid) {
  start_count(threadid);
}

void Task_Profiler::TP_CaptureReturn(THREADID threadid) {
  size_t count = stop_n_get_count(threadid);
  struct AFTask* cur_step = taskGraph->getCurStep(threadid);
  cur_step->t_prof.work += count;
}

void Task_Profiler::TP_CaptureWait_Entry(THREADID threadid) {
  size_t count = stop_n_get_count(threadid);
  struct AFTask* cur_step = taskGraph->getCurStep(threadid);
  cur_step->t_prof.work += count;
}

void Task_Profiler::TP_CaptureWait_Exit(THREADID threadid) {
  start_count(threadid);
}

void Task_Profiler::TP_CaptureSpawn_Entry(THREADID threadid) {
  size_t count = stop_n_get_count(threadid);
  struct AFTask* cur_step = taskGraph->getCurStep(threadid);
  cur_step->t_prof.work += count;
}

void Task_Profiler::TP_CaptureSpawn_Exit(THREADID threadid) {
  start_count(threadid);
}

void Task_Profiler::TP_CaptureBeginOptimize(THREADID threadid, const char* file, int line, void* return_address) {
  //stop performance counter and update count in step node
  size_t count = stop_n_get_count(threadid);
  struct AFTask* cur_step = taskGraph->getCurStep(threadid);
  cur_step->t_prof.work += count;

  //start count again
  start_count(threadid);
}

void Task_Profiler::TP_CaptureEndOptimize(THREADID threadid, const char* file, int line, void* return_address) {
  //stop performance counter and update count in step node
  size_t count = stop_n_get_count(threadid);
  struct AFTask* cur_step = taskGraph->getCurStep(threadid);
  cur_step->t_prof.work += count;

  //add region address to region map if not present
  if (regionMap.count((ADDRINT)return_address) == 0) {
    struct CallSiteData* callsiteData = new CallSiteData();
    callsiteData->cs_filename = file;
    callsiteData->cs_line_number = line;
    regionMap.insert(std::pair<size_t, struct CallSiteData*>((ADDRINT)return_address, callsiteData));
  }

  //add region work to step map
  if (cur_step->t_prof.region_work == NULL) {
    //intialize map
    cur_step->t_prof.region_work = new std::map<size_t, size_t>();
    (cur_step->t_prof.region_work)->insert( std::pair<size_t, size_t>((ADDRINT)return_address, count) );
  } else {
    std::map<size_t, size_t>* rw_map = cur_step->t_prof.region_work;
    if (rw_map->count((ADDRINT)return_address) == 0) {
      rw_map->insert( std::pair<size_t, size_t>((ADDRINT)return_address, count) );
    } else {
      rw_map->at((ADDRINT)return_address) += count;
    }
  }

  //start count again
  start_count(threadid);
}

void Task_Profiler::Fini() {
  size_t count = stop_n_get_count(0);
  struct AFTask* cur_step = taskGraph->getCurStep(0);
  cur_step->t_prof.work += count;

  report.open("ws_profile.csv");
  std::map<size_t, struct WorkSpanData> workSpanMap;
  struct AFTask* head = taskGraph->getHead();
  calculateRecurse(head, &workSpanMap);

  report << "Source file,Line number,Work,Span,Parallelism,Critical path percent" << std::endl;

  report << "main" << ","
	 << 0 << ","
    //<< 1 << ","
	 << head->t_prof.work << ","
	 << head->t_prof.critical_child << ","
	 << (double)head->t_prof.work/(double)head->t_prof.critical_child << ","
    //<< head->t_prof.local_local_work << ","
	 << ((double)head->t_prof.local_local_work/(double)head->t_prof.critical_child)*100.00
	 << std::endl;

  std::map<size_t,size_t>* head_cs_data = head->t_prof.critical_call_sites;
  size_t check_critical_work = head->t_prof.local_local_work;

  for (std::map<size_t,struct WorkSpanData>::iterator it=workSpanMap.begin();
       it!=workSpanMap.end(); ++it) {
    struct CallSiteData* callsiteData = taskGraph->getSourceFileAndLine(it->first);
    struct WorkSpanData& workspanData = it->second;
    
    if (head_cs_data->count(it->first) != 0) { //spawn site on critical path
      size_t cs_critical_work;
      if (callsiteData->par_for) {
	cs_critical_work = workspanData.span;
      } else {
	cs_critical_work = head_cs_data->at(it->first);
      }
      check_critical_work += cs_critical_work;

      report << callsiteData->cs_filename << ","
	     << callsiteData->cs_line_number << ","
	//<< (size_t)it->first << ","
	//<< workspanData.call_count << ","
	     << workspanData.work << ","
	     << workspanData.span << ","
	     << (double)workspanData.work/(double)workspanData.span << ","
	//<< cs_critical_work << ","
	     << ((double)cs_critical_work/(double)head->t_prof.critical_child)*100.00
	     << std::endl;
    } else { // spawn site not on critical path
      report << callsiteData->cs_filename << ","
	     << callsiteData->cs_line_number << ","
	//<< (size_t)it->first << ","
	//<< workspanData.call_count << ","
	     << workspanData.work << ","
	     << workspanData.span << ","
	     << (double)workspanData.work/(double)workspanData.span << ","
	//<< 0 << ","
	     << 0
	     << std::endl;
    }
  }

  // for (std::map<size_t,size_t>::iterator it=head_cs_data->begin();
  //      it!=head_cs_data->end(); ++it) {
  //   struct CallSiteData* callsiteData = taskGraph->getSourceFileAndLine(it->first);
    
  //   report << callsiteData->cs_filename << ","
  // 	   << callsiteData->cs_line_number << ","
  // 	   << it->second << ","
  // 	   << ((double)it->second/(double)head->t_prof.critical_child)*100.00
  // 	   << std::endl;
  // }  

  report.close();

  // if (head->t_prof.critical_child != check_critical_work) {
  //   std::cout << "Head critical path = " << head->t_prof.critical_child << std::endl;
  //   std::cout << "Sum critical path = " << check_critical_work << std::endl;

  //   assert(head->t_prof.critical_child == check_critical_work);
  // }

  /********** INCREASE PARALLELISM OF EACH CALLSITE AND REGION **************/

  //report_cs_increase_par(workSpanMap);

  report_region_increase_par();

  /***************************************************************/
}

bool Task_Profiler::recursiveCall(struct AFTask* node) {
  struct AFTask* parent = taskGraph->getTask(node->parent);
  if (parent->call_site != 0) {
    return (parent->call_site == node->call_site);
  } else {
    struct AFTask* parent_parent = taskGraph->getTask(parent->parent);
    return (parent_parent->call_site == node->call_site);
  }
}

void Task_Profiler::calculateRecurse(struct AFTask* node,
				 std::map<size_t, struct WorkSpanData>* workSpanMap) {
  
  size_t* num_processed = new size_t[taskGraph->last_allocated_node+1]();

  for (size_t i = 1; i <= taskGraph->last_allocated_node; i++) {
    struct AFTask* task_node = &taskGraph->tgraph_nodes[i];

    // std::cout << "Type " << task_node->type << ","
    // 	      << "Index " << i << ","
    // 	      << "taskid " << task_node->taskId
    // 	      << std::endl;

    
    /* if all the children have been processed process this node */
    if (task_node->num_children == num_processed[i]) {
      //calculate work span of this node
      calculateWorkSpan(task_node);

      if (task_node->call_site != 0 && !recursiveCall(task_node)) {
	//if ((size_t)task_node->call_site == 4285594) {
	//std::cout << "callsite 198: work = " << task_node->t_prof.work << " spawn = " << task_node->t_prof.critical_child << std::endl;
	//}
	if (workSpanMap->count(task_node->call_site) != 0) {
	  workSpanMap->at(task_node->call_site).work += task_node->t_prof.work;
	  workSpanMap->at(task_node->call_site).span += task_node->t_prof.critical_child;
	  workSpanMap->at(task_node->call_site).call_count++;
	} else {
	  WorkSpanData wsdata;
	  wsdata.work = task_node->t_prof.work;
	  wsdata.span = task_node->t_prof.critical_child;
	  wsdata.call_count = 1;
	  workSpanMap->insert(std::pair<size_t, struct WorkSpanData>(task_node->call_site, wsdata));
	}
      }

      //check if all parents children have been processed
      checkUpdateParentWorkSpan(task_node->parent, num_processed, workSpanMap);
    } else {
      
      //create map for critical call sites
      //std::cout << "creating taskid = " << task_node->taskId << " type = " << task_node->type << std::endl;
      task_node->t_prof.critical_call_sites = new std::map<size_t, size_t>();
      
      if (task_node->type == ASYNC) {
	struct AFTask* parent_node = taskGraph->getTask(task_node->parent);
	task_node->t_prof.parent_work = parent_node->t_prof.local_work;
      }
    } /* if not continue */
  }

  // for (size_t i = 1; i <= taskGraph->last_allocated_node; i++) {
  //   assert(num_processed[i] == taskGraph->tgraph_nodes[i].num_children);
  // }
}

void Task_Profiler::checkUpdateParentWorkSpan(size_t parent_index, 
					      size_t* num_processed,
					      std::map<size_t, struct WorkSpanData>* workSpanMap) {
  num_processed[parent_index]++;
  struct AFTask* parent_node = taskGraph->getTask(parent_index);

  if (parent_node->num_children == num_processed[parent_index]) {
    calculateWorkSpan(parent_node);

    // std::cout << "Type " << parent_node->type << ","
    // 	      << "Index " << parent_index << ","
    // 	      << parent_node->call_site << ","
    // 	      << parent_node->t_prof.work << ","
    // 	      << parent_node->t_prof.critical_child << ","
    // 	      << std::endl;

    if (parent_node->call_site != 0 && !recursiveCall(parent_node)) {
      //if ((size_t)parent_node->call_site == 4321523) {
      //std::cout << "callsite 198: work = " << parent_node->t_prof.work << " spawn = " << parent_node->t_prof.critical_child << std::endl;
      //}
      if (workSpanMap->count(parent_node->call_site) != 0) {
	workSpanMap->at(parent_node->call_site).work += parent_node->t_prof.work;
	workSpanMap->at(parent_node->call_site).span += parent_node->t_prof.critical_child;
	workSpanMap->at(parent_node->call_site).call_count++;
      } else {
	WorkSpanData wsdata;
	wsdata.work = parent_node->t_prof.work;
	wsdata.span = parent_node->t_prof.critical_child;
	wsdata.call_count = 1;
	workSpanMap->insert(std::pair<size_t, struct WorkSpanData>(parent_node->call_site, wsdata));
      }
    }

    if (parent_node->parent != 0) {
      checkUpdateParentWorkSpan(parent_node->parent, num_processed, workSpanMap);
    }
  }
}

void Task_Profiler::calculateWorkSpan(struct AFTask* node) {
  struct AFTask* parent = taskGraph->getTask(node->parent);
  if (node->type == STEP) {
    // Update total work and local work of parent
    parent->t_prof.work += node->t_prof.work;
    parent->t_prof.local_work += node->t_prof.work;
    parent->t_prof.local_local_work += node->t_prof.work;

  } else if (node->type == ASYNC) {
    // Update the work of the parent
    parent->t_prof.work += node->t_prof.work;
    
    // Calculate the span of the subtree with ASYNC node as root
    if (node->t_prof.local_work > node->t_prof.critical_child) {
      node->t_prof.critical_child = node->t_prof.local_work;
    }

    //std::cout << "ASYNC Taskid = " << node->taskId << " work = " << node->t_prof.work << " span = " << node->t_prof.critical_child << std::endl;
    insert_cs_data(node->t_prof.critical_call_sites, node->call_site, node->t_prof.local_local_work);
    
    // Check if ASYNC node realises the greatest span of the parent
    // If it does update the critical_child of the parent to this ASYNC node
    if (node->t_prof.critical_child + node->t_prof.parent_work > parent->t_prof.critical_child) {
      parent->t_prof.critical_child = node->t_prof.critical_child + node->t_prof.parent_work;

      delete parent->t_prof.critical_call_sites;
      parent->t_prof.critical_call_sites = node->t_prof.critical_call_sites;
      node->t_prof.critical_call_sites = NULL;
    }

  } else if (node->type == FINISH) {
     // Update the work of the parent
    parent->t_prof.work += node->t_prof.work;
    
    // Calculate the span of the subtree with FINISH node as root
    if (node->t_prof.local_work > node->t_prof.critical_child) {
      node->t_prof.critical_child = node->t_prof.local_work;

      if (node->sp_root_n_wt_flag == false) {
	delete node->t_prof.critical_call_sites;
	node->t_prof.critical_call_sites = NULL;
	parent->t_prof.local_local_work += node->t_prof.local_local_work;
      } else {
	if (node->call_site != 0 || node->parent != 0) {
	  insert_cs_data(node->t_prof.critical_call_sites, 
			 node->call_site, node->t_prof.local_local_work);
	}
      }
    } else { //needed for just critical path calculation
      std::map<size_t, size_t>* cs_map = node->t_prof.critical_call_sites;
      size_t sum_cs = 0;
      //find sum of all spawn site on critical path
      for (std::map<size_t,size_t>::iterator it=cs_map->begin();
	   it!=cs_map->end(); ++it) {
	sum_cs += it->second;
      }
      parent->t_prof.local_local_work += (node->t_prof.critical_child-sum_cs);
    }

    //std::cout << "FINISH Taskid = " << node->taskId << " work = " << node->t_prof.work << " span = " << node->t_prof.critical_child << std::endl;
    // Add current node's critical path to the parent's critical path
    parent->t_prof.local_work += node->t_prof.critical_child;

    if (node->parent != 0) {
    // add critical call sites to parent and delete current call site
      merge_critical_call_sites(node->t_prof.critical_call_sites, parent->t_prof.critical_call_sites);
    }
  }
}

void Task_Profiler::insert_cs_data(std::map<size_t, size_t>* cs_map, size_t call_site, size_t local_local_work) {
  if (cs_map->count(call_site) != 0) {
    cs_map->at(call_site) += local_local_work;
  } else {
    cs_map->insert(std::pair<size_t, size_t>(call_site, local_local_work));
  }
}

void Task_Profiler::merge_critical_call_sites(std::map<size_t, size_t>* source, 
					      std::map<size_t, size_t>* dest) {

  if (source != NULL) {
    for (std::map<size_t,size_t>::iterator it=source->begin();
	 it!=source->end(); ++it) {
      if (dest->count(it->first) != 0) {//entry present. update
	dest->at(it->first) += it->second;
      } else { //entry not present. add
	dest->insert(std::pair<size_t, size_t>(it->first, it->second));
      }
    }
  }
}

/********** INCREASE PARALLELISM OF EACH CALLSITE **************/

void Task_Profiler::report_cs_increase_par(std::map<size_t,struct WorkSpanData> workSpanMap_orig) {

  unsigned int cs_id = 0;
  for (std::map<size_t,struct WorkSpanData>::iterator it=workSpanMap_orig.begin();
       it!=workSpanMap_orig.end(); ++it) {
    cs_id++;

    std::string u_file_name = "callsite_" + std::to_string(cs_id) + ".csv";
    report.open(u_file_name);
    report << "Call site parallelism factor, Whole program parallelism" << std::endl;

    for (unsigned int i = 0; i < PAR_INC_COUNT; i++) {
      // iterate through all nodes and update critical path
      unsigned int par_increase = PAR_INC_INIT;
      for (unsigned int j = 0; j < i; j++) {
	par_increase = par_increase * PAR_INC_FACTOR;
      }
      update_critical_path(it->first, par_increase);

      // calculate updated work-span
      struct AFTask* head = taskGraph->getHead();
      calculateRecurseRepeat(head, it->first);

      // write result (generate full profile to verify first)
      report << par_increase << ","
	     << (double)head->t_prof.work/(double)head->t_prof.critical_child
	     << std::endl;
    }

    report.close();
  }
}

void Task_Profiler::update_critical_path(size_t ret_addr, unsigned int par_increase) {

  for (size_t i = 1; i <= taskGraph->last_allocated_node; i++) {
    if (taskGraph->tgraph_nodes[i].type == STEP) continue;

    if (taskGraph->tgraph_nodes[i].call_site == ret_addr) {
      taskGraph->tgraph_nodes[i].t_prof.critical_child /= par_increase;
    } else {
      taskGraph->tgraph_nodes[i].t_prof.critical_child = 0;
      taskGraph->tgraph_nodes[i].t_prof.local_work = 0;
      taskGraph->tgraph_nodes[i].t_prof.parent_work = 0;
    }
  }

}

void Task_Profiler::calculateRecurseRepeat(struct AFTask* node,
					   size_t updated_cs) {
  
  size_t* num_processed = new size_t[taskGraph->last_allocated_node+1]();

  for (size_t i = 1; i <= taskGraph->last_allocated_node; i++) {
    struct AFTask* task_node = &taskGraph->tgraph_nodes[i];
    
    /* if all the children have been processed process this node */
    if (task_node->num_children == num_processed[i]) {
      //calculate work span of this node
      calculateWorkSpanRepeat(task_node, updated_cs);

      //check if all parents children have been processed
      checkUpdateParentWorkSpanRepeat(task_node->parent, num_processed, updated_cs);
    } else if (task_node->type == ASYNC) {
      struct AFTask* parent_node = taskGraph->getTask(task_node->parent);
      task_node->t_prof.parent_work = parent_node->t_prof.local_work;
    } /* if not continue */
  }
}

void Task_Profiler::checkUpdateParentWorkSpanRepeat(size_t parent_index, 
						    size_t* num_processed,
						    size_t updated_cs) {
  num_processed[parent_index]++;
  struct AFTask* parent_node = taskGraph->getTask(parent_index);

  if (parent_node->num_children == num_processed[parent_index]) {
    calculateWorkSpanRepeat(parent_node, updated_cs);

    // std::cout << "Type " << parent_node->type << ","
    // 	      << "Index " << parent_index << ","
    // 	      << parent_node->t_prof.local_work << ","
    // 	      << parent_node->t_prof.critical_child << ","
    // 	      << std::endl;

    if (parent_node->parent != 0) {
      checkUpdateParentWorkSpanRepeat(parent_node->parent, num_processed, updated_cs);
    }
  }
}

void Task_Profiler::calculateWorkSpanRepeat(struct AFTask* node, size_t updated_cs) {
  struct AFTask* parent = taskGraph->getTask(node->parent);
  if (node->type == STEP) {
    // Update total work and local work of parent
    //parent->t_prof.work += node->t_prof.work;
    parent->t_prof.local_work += node->t_prof.work;

  } else if (node->type == ASYNC) {
    // Update the work of the parent
    //parent->t_prof.work += node->t_prof.work;
    
    if (node->call_site != updated_cs) {
      // Calculate the span of the subtree with ASYNC node as root
      if (node->t_prof.local_work > node->t_prof.critical_child) {
	node->t_prof.critical_child = node->t_prof.local_work;
      }
    }    

    if (parent->call_site != updated_cs) {
      // Check if ASYNC node realises the greatest span of the parent
      // If it does update the critical_child of the parent to this ASYNC node
      if (node->t_prof.critical_child + node->t_prof.parent_work > parent->t_prof.critical_child) {
	parent->t_prof.critical_child = node->t_prof.critical_child + node->t_prof.parent_work;
      }
    }

  } else if (node->type == FINISH) {
     // Update the work of the parent
    //parent->t_prof.work += node->t_prof.work;
    
    if (node->call_site != updated_cs) {
      // Calculate the span of the subtree with FINISH node as root
      if (node->t_prof.local_work > node->t_prof.critical_child) {
	node->t_prof.critical_child = node->t_prof.local_work;
      }
    }

    if (parent->call_site != updated_cs) {
      // Add current node's critical path to the parent's critical path
      parent->t_prof.local_work += node->t_prof.critical_child;
    }
  }    
}

/***************************************************************/


/********** INCREASE PARALLELISM OF EACH STATIC REGION **************/

void Task_Profiler::report_region_increase_par() {
  unsigned int cs_id = 0;
  for (std::map<size_t,struct CallSiteData*>::iterator it=regionMap.begin();
       it!=regionMap.end(); ++it) {
    cs_id++;

    std::string u_file_name = "region_" + std::to_string(cs_id) + ".csv";
    report.open(u_file_name);

    struct CallSiteData* callsiteData = it->second;    
    report << callsiteData->cs_filename << ","
	   << callsiteData->cs_line_number << std::endl;

    report << "Region Optimization factor, Whole program parallelism" << std::endl;

    for (unsigned int i = 0; i < PAR_INC_COUNT; i++) {
      reset_work_span();

      // iterate through all nodes and update critical path
      unsigned int par_increase = PAR_INC_INIT;
      for (unsigned int j = 0; j < i; j++) {
	par_increase = par_increase * PAR_INC_FACTOR;
      }

      // calculate updated work-span
      struct AFTask* head = taskGraph->getHead();
      calculateRecurse_region(head, it->first, par_increase);

      // write result (generate full profile to verify first)
      report << par_increase << ","
	     // << head->t_prof.work << ","
	     // << head->t_prof.critical_child << ","
	     << (double)head->t_prof.work/(double)head->t_prof.critical_child
	     << std::endl;
    }

    report.close();
  }

  if (!regionMap.empty()) {
    //Optimize all regions, combined
    std::string u_file_name = "region_all.csv";
    report.open(u_file_name);

    report << "Regions Optimization factor, Whole program parallelism" << std::endl;

    for (unsigned int i = 0; i < PAR_INC_COUNT; i++) {
      reset_work_span();

      // iterate through all nodes and update critical path
      unsigned int par_increase = PAR_INC_INIT;
      for (unsigned int j = 0; j < i; j++) {
	par_increase = par_increase * PAR_INC_FACTOR;
      }

      // calculate updated work-span
      struct AFTask* head = taskGraph->getHead();
      calculateRecurse_region(head, 0, par_increase);

      // write result (generate full profile to verify first)
      report << par_increase << ","
	     // << head->t_prof.work << ","
	     // << head->t_prof.critical_child << ","
	     << (double)head->t_prof.work/(double)head->t_prof.critical_child
	     << std::endl;
    }

    report.close();
  }

}

void Task_Profiler::reset_work_span() {

  for (size_t i = 1; i <= taskGraph->last_allocated_node; i++) {
    if (taskGraph->tgraph_nodes[i].type == STEP) continue;

    //taskGraph->tgraph_nodes[i].t_prof.work = 0;
    taskGraph->tgraph_nodes[i].t_prof.critical_child = 0;
    taskGraph->tgraph_nodes[i].t_prof.local_work = 0;
    taskGraph->tgraph_nodes[i].t_prof.parent_work = 0;
  }

}

void Task_Profiler::calculateRecurse_region(struct AFTask* node,
					    size_t updated_region,
					    unsigned int par_increase) {
  
  size_t* num_processed = new size_t[taskGraph->last_allocated_node+1]();

  for (size_t i = 1; i <= taskGraph->last_allocated_node; i++) {
    struct AFTask* task_node = &taskGraph->tgraph_nodes[i];
    
    /* if all the children have been processed process this node */
    if (task_node->num_children == num_processed[i]) {
      //calculate work span of this node
      calculateWorkSpan_region(task_node, updated_region, par_increase);

      //check if all parents children have been processed
      checkUpdateParentWorkSpan_region(task_node->parent, num_processed, updated_region, par_increase);
    } else if (task_node->type == ASYNC) {
      struct AFTask* parent_node = taskGraph->getTask(task_node->parent);
      task_node->t_prof.parent_work = parent_node->t_prof.local_work;
    } /* if not continue */
  }
}

void Task_Profiler::checkUpdateParentWorkSpan_region(size_t parent_index, 
						    size_t* num_processed,
						     size_t updated_region,
						     unsigned int par_increase) {
  num_processed[parent_index]++;
  struct AFTask* parent_node = taskGraph->getTask(parent_index);

  if (parent_node->num_children == num_processed[parent_index]) {
    calculateWorkSpan_region(parent_node, updated_region, par_increase);

    if (parent_node->parent != 0) {
      checkUpdateParentWorkSpan_region(parent_node->parent, num_processed, updated_region, par_increase);
    }
  }
}

void Task_Profiler::calculateWorkSpan_region(struct AFTask* node, size_t updated_region, unsigned int par_increase) {
  struct AFTask* parent = taskGraph->getTask(node->parent);
  if (node->type == STEP) {
    // Update total work and local work of parent
    size_t step_work = node->t_prof.work;
    if (node->t_prof.region_work != NULL) {
      if (updated_region == 0) { //optimize all regions
	for (std::map<size_t,size_t>::iterator it=(node->t_prof.region_work)->begin();
	     it!=(node->t_prof.region_work)->end(); ++it) {
	  size_t region_work = it->second;
	  //std::cout << "Work of region = " << region_work << std::endl;
	  size_t region_work_decrease = region_work - (region_work/par_increase);
	  step_work -= region_work_decrease;
	  //std::cout << "reduced step work by " << region_work_decrease << std::endl;	  
	}
	
      } else if (node->t_prof.region_work->count(updated_region) != 0) {
	size_t region_work = node->t_prof.region_work->at(updated_region);
	//std::cout << "Work of region = " << region_work << std::endl;
	size_t region_work_decrease = region_work - (region_work/par_increase);
	step_work -= region_work_decrease;
	//std::cout << "reduced step work by " << region_work_decrease << std::endl;
      }
    }

    //parent->t_prof.work += step_work;
    parent->t_prof.local_work += step_work;

  } else if (node->type == ASYNC) {
    // Update the work of the parent
    //parent->t_prof.work += node->t_prof.work;
    
    // Calculate the span of the subtree with ASYNC node as root
    if (node->t_prof.local_work > node->t_prof.critical_child) {
      node->t_prof.critical_child = node->t_prof.local_work;
    }

    // Check if ASYNC node realises the greatest span of the parent
    // If it does update the critical_child of the parent to this ASYNC node
    if (node->t_prof.critical_child + node->t_prof.parent_work > parent->t_prof.critical_child) {
      parent->t_prof.critical_child = node->t_prof.critical_child + node->t_prof.parent_work;
    }

  } else if (node->type == FINISH) {
    // Update the work of the parent
    //parent->t_prof.work += node->t_prof.work;
    
    // Calculate the span of the subtree with FINISH node as root
    if (node->t_prof.local_work > node->t_prof.critical_child) {
      node->t_prof.critical_child = node->t_prof.local_work;
    }

    // Add current node's critical path to the parent's critical path
    parent->t_prof.local_work += node->t_prof.critical_child;
  }
}

/***************************************************************/
