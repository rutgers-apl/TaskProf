#include<assert.h>
#include "Task_Logger.H"

Task_Logger::Task_Logger() {
  for (unsigned int i = 0; i < NUM_THREADS; i++) {
    last_allocated[i] = 0;
  }

  for (unsigned int i = 0; i < NUM_THREADS; i++) {
    std::string file_name = "step_work_" + std::to_string(i) + ".csv";
    report[i].open(file_name);
  }
}

void Task_Logger::log(THREADID threadid,
		      size_t node_id,
		      size_t parent,
		      size_t work,
		      unsigned short int node_type,
		      ADDRINT ret_addr) {
  
  if (last_allocated[threadid] == NUM_ENTRIES) {
    //write to file
    for (unsigned int i = 0; i < NUM_ENTRIES; i++) {
      report[threadid] << step_work_list[threadid][i].node_id << ","
		       << step_work_list[threadid][i].step_parent << ","
		       << step_work_list[threadid][i].node_type << ","
		       << step_work_list[threadid][i].ret_addr << ","
		       << step_work_list[threadid][i].work;

      if (step_work_list[threadid][i].region_work != NULL) {
	assert(step_work_list[threadid][i].node_type == 3);
	for (std::unordered_map<size_t, size_t>::iterator it=step_work_list[threadid][i].region_work->begin();
	     it!=step_work_list[threadid][i].region_work->end(); ++it) {
	  report[threadid] << "," << it->first << "," << it->second;
	}
      }

      report[threadid] << std::endl;
    }

    //updated last_allocated to 0
    last_allocated[threadid] = 0;
    step_work_list[threadid][last_allocated[threadid]].work = 0;
    if (step_work_list[threadid][last_allocated[threadid]].region_work) {
      delete step_work_list[threadid][last_allocated[threadid]].region_work;
      step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
    }
  }

  step_work_list[threadid][last_allocated[threadid]].node_id = node_id;
  step_work_list[threadid][last_allocated[threadid]].step_parent = parent;
  step_work_list[threadid][last_allocated[threadid]].node_type = node_type;
  step_work_list[threadid][last_allocated[threadid]].ret_addr = ret_addr;
  step_work_list[threadid][last_allocated[threadid]].node_id = node_id;

  step_work_list[threadid][last_allocated[threadid]].work += work;

  last_allocated[threadid]++;
  step_work_list[threadid][last_allocated[threadid]].work = 0;
  if (step_work_list[threadid][last_allocated[threadid]].region_work) {
    delete step_work_list[threadid][last_allocated[threadid]].region_work;
    step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
  }  
}

void Task_Logger::log_intermediate_step(THREADID threadid,
		      size_t node_id,
		      size_t parent,
		      size_t work,
		      unsigned short int node_type,
		      ADDRINT ret_addr) {
  
  if (last_allocated[threadid] == NUM_ENTRIES) {
    //write to file
    for (unsigned int i = 0; i < NUM_ENTRIES; i++) {
      report[threadid] << step_work_list[threadid][i].node_id << ","
		       << step_work_list[threadid][i].step_parent << ","
		       << step_work_list[threadid][i].node_type << ","
		       << step_work_list[threadid][i].ret_addr << ","
		       << step_work_list[threadid][i].work;

      if (step_work_list[threadid][i].region_work != NULL) {
	assert(step_work_list[threadid][i].node_type == 3);
	for (std::unordered_map<size_t, size_t>::iterator it=step_work_list[threadid][i].region_work->begin();
	     it!=step_work_list[threadid][i].region_work->end(); ++it) {
	  report[threadid] << "," << it->first << "," << it->second;
	}
      }

      report[threadid] << std::endl;
    }

    //updated last_allocated to 0
    last_allocated[threadid] = 0;
    step_work_list[threadid][last_allocated[threadid]].work = 0;
    if (step_work_list[threadid][last_allocated[threadid]].region_work) {
      delete step_work_list[threadid][last_allocated[threadid]].region_work;
      step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
    }
  }

  step_work_list[threadid][last_allocated[threadid]].node_id = node_id;
  step_work_list[threadid][last_allocated[threadid]].step_parent = parent;
  step_work_list[threadid][last_allocated[threadid]].node_type = node_type;
  step_work_list[threadid][last_allocated[threadid]].ret_addr = ret_addr;
  step_work_list[threadid][last_allocated[threadid]].node_id = node_id;

  step_work_list[threadid][last_allocated[threadid]].work += work;
}

void Task_Logger::buffer_info(THREADID threadid,
			      size_t node_id,
			      size_t parent,
			      size_t work,
			      int node_type,
			      ADDRINT ret_addr) {
  step_work_list[threadid][last_allocated[threadid]].node_id = node_id;
  step_work_list[threadid][last_allocated[threadid]].step_parent = parent;
  step_work_list[threadid][last_allocated[threadid]].node_type = node_type;
  step_work_list[threadid][last_allocated[threadid]].ret_addr = ret_addr;
  step_work_list[threadid][last_allocated[threadid]].node_id = node_id;  
  step_work_list[threadid][last_allocated[threadid]].work += work;  
}

void Task_Logger::log_region_work(THREADID threadid,ADDRINT return_address,size_t work) {
  assert(step_work_list[threadid][last_allocated[threadid]].node_type == 3);
  if (step_work_list[threadid][last_allocated[threadid]].region_work == NULL) {
    step_work_list[threadid][last_allocated[threadid]].region_work = new std::unordered_map<size_t, size_t>();
    (step_work_list[threadid][last_allocated[threadid]].region_work)->insert( std::pair<size_t, size_t>(return_address, work) );
  } else {
    std::unordered_map<size_t, size_t>* rw_map = step_work_list[threadid][last_allocated[threadid]].region_work;
    if (rw_map->count(return_address) == 0) {
      rw_map->insert( std::pair<size_t, size_t>(return_address, work) );
    } else {
      rw_map->at(return_address) += work;
    }
  }
}

void Task_Logger::Fini(size_t node_id,
		       size_t parent,
		       size_t work,
		       unsigned short int node_type,
		       ADDRINT ret_addr) {
  for (unsigned int threadid = 0; threadid < NUM_THREADS; threadid++) {
    for (unsigned int i = 0; i < last_allocated[threadid]; i++) {
      report[threadid] << step_work_list[threadid][i].node_id << ","
		       << step_work_list[threadid][i].step_parent << ","
		       << step_work_list[threadid][i].node_type << ","
		       << step_work_list[threadid][i].ret_addr << ","
		       << step_work_list[threadid][i].work;


      if (step_work_list[threadid][i].region_work != NULL) {
	for (std::unordered_map<size_t, size_t>::iterator it=step_work_list[threadid][i].region_work->begin();
	     it!=step_work_list[threadid][i].region_work->end(); ++it) {
	  report[threadid] << "," << it->first << "," << it->second;
	}
      }
      
      report[threadid] << std::endl;
    }
  }

  report[0] << node_id << ","
	    << parent << ","
	    << node_type << ","
	    << ret_addr << ","
	    << step_work_list[0][last_allocated[0]].work + work;
  
  if (step_work_list[0][last_allocated[0]].region_work != NULL) {
    for (std::unordered_map<size_t, size_t>::iterator it=step_work_list[0][last_allocated[0]].region_work->begin();
	 it!=step_work_list[0][last_allocated[0]].region_work->end(); ++it) {
      report[0] << "," << it->first << "," << it->second;
    }
  }

  report[0] << std::endl;

  for (unsigned int i = 0; i < NUM_THREADS; i++) {
    report[i].close();
  }
}
