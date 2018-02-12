#include <string.h>
#include <sstream>
#include <fstream>
#include <sys/stat.h>

#include "Task_Profiler.H"
#include "CallSiteData.H"

#define PAR_INC_COUNT 4
#define PAR_INC_INIT 50
#define PAR_INC_FACTOR 2

Task_Profiler::Task_Profiler() {
  taskGraph = new AFTaskGraph(".");

  std::ifstream region_file;
  region_file.open("region_info.csv");
  if (region_file.is_open()) {
    std::string line;      
    while (std::getline (region_file, line)) {
      std::stringstream sstream(line);
      std::string cs_str;
      std::getline(sstream, cs_str, ',');
      std::string fn_str;
      std::getline(sstream, fn_str, ',');
      std::string line_str;
      std::getline(sstream, line_str, ',');
      
      struct CallSiteData* cs_data = new CallSiteData();
      cs_data->cs_filename = fn_str;
      cs_data->cs_line_number = std::stoi(line_str);
	
      regionMap.insert(std::pair<size_t, struct CallSiteData*>(std::stoul(cs_str), cs_data));
    }
  }
  GenerateProfile(".");
}

size_t Task_Profiler::GenerateProfile(std::string folder) {
  workSpanMap = new std::unordered_map<size_t, struct WorkSpanData>();
  struct AFTask* head = taskGraph->getHead();
  calculateRecurse(head, workSpanMap);

  std::ofstream report;
  report.open(folder + "/ws_profile.csv");  
  report << "Source file,Line number,Work,Span,Parallelism,Percent critical work" << std::endl;

  report << "main" << ","
	 << 0 << ","
	 << head->t_prof.work << ","
	 << head->t_prof.critical_child << ","
	 << (double)head->t_prof.work/(double)head->t_prof.critical_child << ","
	 << ((double)head->t_prof.local_local_work/(double)head->t_prof.critical_child)*100.00
	 << std::endl;

  std::unordered_map<size_t,size_t>* head_cs_data = head->t_prof.critical_call_sites;
  size_t check_critical_work = head->t_prof.local_local_work;

  for (std::unordered_map<size_t,struct WorkSpanData>::iterator it=workSpanMap->begin();
       it!=workSpanMap->end(); ++it) {
    struct CallSiteData* callsiteData = taskGraph->getSourceFileAndLine(it->first, folder);
    struct WorkSpanData& workspanData = it->second;
    
    if (head_cs_data->count(it->first) != 0) { //spawn site on critical path
      size_t cs_critical_work = head_cs_data->at(it->first);
      check_critical_work += cs_critical_work;

      report << callsiteData->cs_filename << ","
	     << callsiteData->cs_line_number << ","
	     << workspanData.work << ","
	     << workspanData.span << ","
	     << (double)workspanData.work/(double)workspanData.span << ","
	     << ((double)cs_critical_work/(double)head->t_prof.critical_child)*100.00
	     << std::endl;
    } else { // spawn site not on critical path
      report << callsiteData->cs_filename << ","
	     << callsiteData->cs_line_number << ","
	     << workspanData.work << ","
	     << workspanData.span << ","
	     << (double)workspanData.work/(double)workspanData.span << ","
	     << 0
	     << std::endl;
    }
  }
  
  report.close();

  size_t total_work = head->t_prof.work;
  /********** INCREASE PARALLELISM OF EACH CALLSITE AND REGION **************/
  
  report_region_increase_par();

  /***************************************************************/
  return total_work;
}

bool Task_Profiler::checkRecursiveCall(struct AFTask* node, size_t call_site) {
  if (node->parent == 0) {
    return false;
  } else {
    struct AFTask* parent = taskGraph->getTask(node->parent);
    if (parent->call_site == node->call_site) {
      return true;
    } else {
      return checkRecursiveCall(parent, call_site);
    }
  }
}

bool Task_Profiler::recursiveCall(struct AFTask* node) {
  return checkRecursiveCall(node, node->call_site);
}

void Task_Profiler::calculateRecurse(struct AFTask* node,
				 std::unordered_map<size_t, struct WorkSpanData>* workSpanMap) {
  
  size_t* num_processed = new size_t[taskGraph->last_allocated_node+1]();

  for (size_t i = 1; i <= taskGraph->last_allocated_node; i++) {
    struct AFTask* task_node = &taskGraph->tgraph_nodes[i];
    
    /* if all the children have been processed process this node */
    if (task_node->num_children == num_processed[i]) {
      //calculate work span of this node
      calculateWorkSpan(task_node);

      /* If node is a ASYNC node and is not a recursive call */
      if (task_node->call_site != 0 && !recursiveCall(task_node)) {
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
      task_node->t_prof.critical_call_sites = new std::unordered_map<size_t, size_t>();
      
      if (task_node->type == ASYNC) {
	struct AFTask* parent_node = taskGraph->getTask(task_node->parent);
	task_node->t_prof.parent_work = parent_node->t_prof.local_work;
      }
    }
  }
}

void Task_Profiler::checkUpdateParentWorkSpan(size_t parent_index, 
					      size_t* num_processed,
					      std::unordered_map<size_t, struct WorkSpanData>* workSpanMap) {
  num_processed[parent_index]++;
  struct AFTask* parent_node = taskGraph->getTask(parent_index);

  if (parent_node->num_children == num_processed[parent_index]) {
    calculateWorkSpan(parent_node);

    if (parent_node->call_site != 0 && !recursiveCall(parent_node)) {
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
    if (node->t_prof.local_work >= node->t_prof.critical_child) {
      node->t_prof.critical_child = node->t_prof.local_work;
    }

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
    if (node->t_prof.local_work >= node->t_prof.critical_child) {
      node->t_prof.critical_child = node->t_prof.local_work;

      if (node->sp_root_n_wt_flag == true) { //if the finish node is created from a spawn_root_and_wait call
	struct CallSiteData* callsiteData = taskGraph->getSourceFileAndLine(node->call_site, ".");
	if (callsiteData->par_for) {
	  node->t_prof.critical_call_sites->clear();
	  insert_cs_data(node->t_prof.critical_call_sites, 
			 node->call_site, node->t_prof.critical_child);
	} else {
	  insert_cs_data(node->t_prof.critical_call_sites, 
			 node->call_site, node->t_prof.local_local_work);
	}
      } else { //if implicit finish node or head finish node
	if (node->parent != 0) {
	  delete node->t_prof.critical_call_sites;
	  node->t_prof.critical_call_sites = NULL;
	  parent->t_prof.local_local_work += node->t_prof.local_local_work;
	}
      }
    } else { //needed for just critical path calculation
      std::unordered_map<size_t, size_t>* cs_map = node->t_prof.critical_call_sites;
      size_t sum_cs = 0;
      //find sum of all spawn site on critical path
      for (std::unordered_map<size_t,size_t>::iterator it=cs_map->begin();
	   it!=cs_map->end(); ++it) {
	sum_cs += it->second;
      }
      parent->t_prof.local_local_work += (node->t_prof.critical_child-sum_cs);
    }

    // Add current node's critical path to the parent's critical path
    parent->t_prof.local_work += node->t_prof.critical_child;

    if (node->parent != 0) {
      // add critical call sites to parent and delete current call site
      merge_critical_call_sites(node->t_prof.critical_call_sites, parent->t_prof.critical_call_sites);
    }
  }
}

void Task_Profiler::insert_cs_data(std::unordered_map<size_t, size_t>* cs_map, size_t call_site, size_t local_local_work) {
  if (cs_map->count(call_site) != 0) {
    cs_map->at(call_site) += local_local_work;
  } else {
    cs_map->insert(std::pair<size_t, size_t>(call_site, local_local_work));
  }
}

void Task_Profiler::merge_critical_call_sites(std::unordered_map<size_t, size_t>* source, 
					      std::unordered_map<size_t, size_t>* dest) {

  if (source != NULL) {
    for (std::unordered_map<size_t,size_t>::iterator it=source->begin();
	 it!=source->end(); ++it) {
      if (dest->count(it->first) != 0) {//entry present. update
	dest->at(it->first) += it->second;
      } else { //entry not present. add
	dest->insert(std::pair<size_t, size_t>(it->first, it->second));
      }
    }
  }
}

/********** INCREASE PARALLELISM OF EACH STATIC REGION **************/

void Task_Profiler::report_region_increase_par() {
  unsigned int cs_id = 0;
  for (std::unordered_map<size_t,struct CallSiteData*>::iterator it=regionMap.begin();
       it!=regionMap.end(); ++it) {
    cs_id++;

    std::ofstream report;
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
    std::ofstream report;
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
	for (std::unordered_map<size_t,size_t>::iterator it=(node->t_prof.region_work)->begin();
	     it!=(node->t_prof.region_work)->end(); ++it) {
	  size_t region_work = it->second;
	  size_t region_work_decrease = region_work - (region_work/par_increase);
	  step_work -= region_work_decrease;
	}
	
      } else if (node->t_prof.region_work->count(updated_region) != 0) {
	size_t region_work = node->t_prof.region_work->at(updated_region);
	size_t region_work_decrease = region_work - (region_work/par_increase);
	step_work -= region_work_decrease;
      }
    }

    //parent->t_prof.work += step_work;
    parent->t_prof.local_work += step_work;

  } else if (node->type == ASYNC) {
    // Update the work of the parent
    //parent->t_prof.work += node->t_prof.work;
    
    // Calculate the span of the subtree with ASYNC node as root
    if (node->t_prof.local_work >= node->t_prof.critical_child) {
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
    if (node->t_prof.local_work >= node->t_prof.critical_child) {
      node->t_prof.critical_child = node->t_prof.local_work;
    }

    // Add current node's critical path to the parent's critical path
    parent->t_prof.local_work += node->t_prof.critical_child;
  }
}

/***************************************************************/
