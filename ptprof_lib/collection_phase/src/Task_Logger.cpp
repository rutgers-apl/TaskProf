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

void Task_Logger::update_step_and_region(THREADID threadid, struct location start, struct location end) {
  update_file_line(threadid,start,end);
  
  last_allocated[threadid]++;
  step_work_list[threadid][last_allocated[threadid]].work = 0;
  if (step_work_list[threadid][last_allocated[threadid]].region_work) {
    delete step_work_list[threadid][last_allocated[threadid]].region_work;
    step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
  }
}

void Task_Logger::update_file_line(THREADID threadid,struct location start, struct location end) {
  if(start.filename != NULL) {
    step_work_list[threadid][last_allocated[threadid]].start.line = start.line;
    step_work_list[threadid][last_allocated[threadid]].start.filename = new char[64];
    strcpy(step_work_list[threadid][last_allocated[threadid]].start.filename, start.filename);
    if(end.filename != NULL) {
      step_work_list[threadid][last_allocated[threadid]].end.line = end.line;
      step_work_list[threadid][last_allocated[threadid]].end.filename = new char[64];
      strcpy(step_work_list[threadid][last_allocated[threadid]].end.filename, end.filename);
    }    
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
		       << step_work_list[threadid][i].work;// << ","

      if (step_work_list[threadid][i].start.filename != NULL) {
	report[threadid] << "," << step_work_list[threadid][i].start.line
			 << "," << step_work_list[threadid][i].start.filename;
	delete step_work_list[threadid][i].start.filename;
	step_work_list[threadid][i].start.filename = NULL;
      }
      
      if(step_work_list[threadid][i].end.filename != NULL) {
	report[threadid] << "," << step_work_list[threadid][i].end.line
			 << "," << step_work_list[threadid][i].end.filename;
	delete step_work_list[threadid][i].end.filename;
	step_work_list[threadid][i].end.filename = NULL;
      }

      if (step_work_list[threadid][i].region_work != NULL) {
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

  if (node_type == 1 || node_type == 2) { //if async or finish node
    last_allocated[threadid]++;
    step_work_list[threadid][last_allocated[threadid]].work = 0;
  }
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
		       ADDRINT ret_addr,
		       struct location start,
		       struct location end) {
  for (unsigned int threadid = 0; threadid < NUM_THREADS; threadid++) {
    for (unsigned int i = 0; i < last_allocated[threadid]; i++) {
      report[threadid] << step_work_list[threadid][i].node_id << ","
		       << step_work_list[threadid][i].step_parent << ","
		       << step_work_list[threadid][i].node_type << ","
		       << step_work_list[threadid][i].ret_addr << ","
		       << step_work_list[threadid][i].work;// << ","
      //<< step_work_list[threadid][i].seq_num_str;

      if (step_work_list[threadid][i].start.filename != NULL) {
	report[threadid] << "," << step_work_list[threadid][i].start.line
			 << "," << step_work_list[threadid][i].start.filename;
	delete step_work_list[threadid][i].start.filename;
	step_work_list[threadid][i].start.filename = NULL;	
      }

      if(step_work_list[threadid][i].end.filename != NULL) {
	report[threadid] << "," << step_work_list[threadid][i].end.line
			 << "," << step_work_list[threadid][i].end.filename;
	delete step_work_list[threadid][i].end.filename;
	step_work_list[threadid][i].end.filename = NULL;
      }

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
	    << step_work_list[0][last_allocated[0]].work + work << ","
	    << start.line << ","
	    << start.filename << ","
	    << end.line << ","
	    << end.filename;

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
