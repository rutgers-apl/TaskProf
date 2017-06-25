#include <iostream>
#include <string.h>

#include "unique_task_id.H"
#include "Task_Profiler.H"
//#include "CallSiteData.H"

Task_Profiler::Task_Profiler() {
  for (unsigned int i = 0; i < NUM_THREADS; i++) {
    last_allocated[i] = 0;
  }

  for (unsigned int i = 0; i < NUM_THREADS; i++) {
    std::string file_name = "step_work_" + std::to_string(i) + ".csv";
    report[i].open(file_name);
  }

  //report.open("step_work.csv");
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
    fprintf(stderr, "Unable to read performance counters. Linux perf event API not supported on the machine. Error number: %d\n", errno);
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

  //size_t cur_step = taskGraph->getCurStep(threadid);
  //char* sq_str = new char[cur_step.seq_num_str.length()+1];
  //std::copy(cur_step.seq_num_str.begin(), cur_step.seq_num_str.end(), sq_str);
  //sq_str[cur_step.seq_num_str.length()] = '\0';
  //cur_step_data.seq_num_str = sq_str;
  //cur_step_data.depth = cur_step.depth;

  // struct step_work_data cur_step_data;
  // cur_step_data.step_parent = taskGraph->getCurStep(threadid);
  // cur_step_data.work = count;
  
  if (last_allocated[threadid] == NUM_ENTRIES) {
    //write to file
    for (unsigned int i = 0; i < NUM_ENTRIES; i++) {
      report[threadid] << step_work_list[threadid][i].step_parent << ","
		       << step_work_list[threadid][i].work;
      if (step_work_list[threadid][i].region_work != NULL) {
	for (std::map<size_t, size_t>::iterator it=step_work_list[threadid][i].region_work->begin();
	     it!=step_work_list[threadid][i].region_work->end(); ++it) {
	  report[threadid]  << "," << it->first << "," << it->second;
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
  step_work_list[threadid][last_allocated[threadid]].step_parent = taskGraph->getCurStep(threadid);
  step_work_list[threadid][last_allocated[threadid]].work += count;
  last_allocated[threadid]++;
  step_work_list[threadid][last_allocated[threadid]].work = 0;
  if (step_work_list[threadid][last_allocated[threadid]].region_work) {
    delete step_work_list[threadid][last_allocated[threadid]].region_work;
    step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
  }

  // struct step_work_data cur_step_data;
  // cur_step_data.seq_num_str = cur_step.seq_num_str;
  // //cur_step_data.depth = cur_step.depth;
  // cur_step_data.work = count;

  // step_work_list.push_back(cur_step_data);
}

void Task_Profiler::TP_CaptureWait_Entry(THREADID threadid) {
  size_t count = stop_n_get_count(threadid);

  // struct unique_task_id cur_step = taskGraph->getCurStep(threadid);
  // char* sq_str = new char[cur_step.seq_num_str.length()+1];
  // std::copy(cur_step.seq_num_str.begin(), cur_step.seq_num_str.end(), sq_str);
  // sq_str[cur_step.seq_num_str.length()] = '\0';
  // cur_step_data.seq_num_str = sq_str;
  // cur_step_data.depth = cur_step.depth;

  // struct step_work_data cur_step_data;
  // cur_step_data.step_parent = taskGraph->getCurStep(threadid);
  // cur_step_data.work = count;
  
  if (last_allocated[threadid] == NUM_ENTRIES) {
    //write to file
    for (unsigned int i = 0; i < NUM_ENTRIES; i++) {
      report[threadid] << step_work_list[threadid][i].step_parent << ","
		       << step_work_list[threadid][i].work;
      if (step_work_list[threadid][i].region_work != NULL) {
	for (std::map<size_t, size_t>::iterator it=step_work_list[threadid][i].region_work->begin();
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
  step_work_list[threadid][last_allocated[threadid]].step_parent = taskGraph->getCurStep(threadid);
  step_work_list[threadid][last_allocated[threadid]].work += count;
  last_allocated[threadid]++;
  step_work_list[threadid][last_allocated[threadid]].work = 0;
  if (step_work_list[threadid][last_allocated[threadid]].region_work) {
    delete step_work_list[threadid][last_allocated[threadid]].region_work;
    step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
  }


  // struct step_work_data cur_step_data;
  // cur_step_data.seq_num_str = cur_step.seq_num_str;
  // //cur_step_data.depth = cur_step.depth;
  // cur_step_data.work = count;

  // step_work_list.push_back(cur_step_data);
}

void Task_Profiler::TP_CaptureWait_Exit(THREADID threadid) {
  start_count(threadid);
}

void Task_Profiler::TP_CaptureSpawn_Entry(THREADID threadid) {
  size_t count = stop_n_get_count(threadid);

  // struct unique_task_id cur_step = taskGraph->getCurStep(threadid);
  // char* sq_str = new char[cur_step.seq_num_str.length()+1];
  // std::copy(cur_step.seq_num_str.begin(), cur_step.seq_num_str.end(), sq_str);
  // sq_str[cur_step.seq_num_str.length()] = '\0';
  // cur_step_data.seq_num_str = sq_str;
  // cur_step_data.depth = cur_step.depth;

  // struct step_work_data cur_step_data;
  // cur_step_data.step_parent = taskGraph->getCurStep(threadid);
  // cur_step_data.work = count;
  
  if (last_allocated[threadid] == NUM_ENTRIES) {
    //write to file
    for (unsigned int i = 0; i < NUM_ENTRIES; i++) {
      report[threadid] << step_work_list[threadid][i].step_parent << ","
		       << step_work_list[threadid][i].work;
      if (step_work_list[threadid][i].region_work != NULL) {
	for (std::map<size_t, size_t>::iterator it=step_work_list[threadid][i].region_work->begin();
	     it!=step_work_list[threadid][i].region_work->end(); ++it) {
	  report[threadid] << "," << it->first << "," << it->second;
	}
      }
      report[threadid] << std::endl;
    }

    //updat last_allocated to 0
    last_allocated[threadid] = 0;
    step_work_list[threadid][last_allocated[threadid]].work = 0;
    if (step_work_list[threadid][last_allocated[threadid]].region_work) {
      delete step_work_list[threadid][last_allocated[threadid]].region_work;
      step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
    }
  }
  step_work_list[threadid][last_allocated[threadid]].step_parent = taskGraph->getCurStep(threadid);
  step_work_list[threadid][last_allocated[threadid]].work += count;
  last_allocated[threadid]++;
  step_work_list[threadid][last_allocated[threadid]].work = 0;
  if (step_work_list[threadid][last_allocated[threadid]].region_work) {
    delete step_work_list[threadid][last_allocated[threadid]].region_work;
    step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
  }

}

void Task_Profiler::TP_CaptureSpawn_Exit(THREADID threadid) {
  start_count(threadid);
}

void Task_Profiler::TP_CaptureBeginOptimize(THREADID threadid, const char* file, int line, void* return_address) {
  size_t count = stop_n_get_count(threadid);  
  if (last_allocated[threadid] == NUM_ENTRIES) {
    //write to file
    for (unsigned int i = 0; i < NUM_ENTRIES; i++) {
      report[threadid] << step_work_list[threadid][i].step_parent << ","
		       << step_work_list[threadid][i].work;
      if (step_work_list[threadid][i].region_work != NULL) {
	for (std::map<size_t, size_t>::iterator it=step_work_list[threadid][i].region_work->begin();
	     it!=step_work_list[threadid][i].region_work->end(); ++it) {
	  report[threadid] << "," << it->first << "," << it->second;
	}
      }
      report[threadid] << std::endl;
    }

    //updat last_allocated to 0
    last_allocated[threadid] = 0;
    step_work_list[threadid][last_allocated[threadid]].work = 0;
    if (step_work_list[threadid][last_allocated[threadid]].region_work) {
      delete step_work_list[threadid][last_allocated[threadid]].region_work;
      step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
    }
  }
  step_work_list[threadid][last_allocated[threadid]].step_parent = taskGraph->getCurStep(threadid);
  step_work_list[threadid][last_allocated[threadid]].work += count;
  //last_allocated[threadid]++;
  //step_work_list[threadid][last_allocated[threadid]].work = 0;

  start_count(threadid);
}

void Task_Profiler::TP_CaptureEndOptimize(THREADID threadid, const char* file, int line, void* return_address) {
  size_t count = stop_n_get_count(threadid);  
  // if (last_allocated[threadid] == NUM_ENTRIES) {
  //   //write to file
  //   for (unsigned int i = 0; i < NUM_ENTRIES; i++) {
  //     report[threadid] << step_work_list[threadid][i].step_parent << ","
  // 		       << step_work_list[threadid][i].work
  // 		       << std::endl;
  //   }

  //   //updat last_allocated to 0
  //   last_allocated[threadid] = 0;
  //   step_work_list[threadid][last_allocated[threadid]].work = 0;
  //   if (step_work_list[threadid][last_allocated[threadid]].region_work) {
  //     delete step_work_list[threadid][last_allocated[threadid]].region_work;
  //     step_work_list[threadid][last_allocated[threadid]].region_work = NULL;
  //   }
  // }
  step_work_list[threadid][last_allocated[threadid]].step_parent = taskGraph->getCurStep(threadid);
  step_work_list[threadid][last_allocated[threadid]].work += count;
  //last_allocated[threadid]++;
  //step_work_list[threadid][last_allocated[threadid]].work = 0;

  if (regionMap.count((ADDRINT)return_address) == 0) {
    struct CallSiteData* callsiteData = new CallSiteData();
    callsiteData->cs_filename = file;
    callsiteData->cs_line_number = line;
    regionMap.insert(std::pair<size_t, struct CallSiteData*>((ADDRINT)return_address, callsiteData));
  }

  if (step_work_list[threadid][last_allocated[threadid]].region_work == NULL) {
    step_work_list[threadid][last_allocated[threadid]].region_work = new std::map<size_t, size_t>();
    (step_work_list[threadid][last_allocated[threadid]].region_work)->insert( std::pair<size_t, size_t>((ADDRINT)return_address, count) );
  } else {
    std::map<size_t, size_t>* rw_map = step_work_list[threadid][last_allocated[threadid]].region_work;
    if (rw_map->count((ADDRINT)return_address) == 0) {
      rw_map->insert( std::pair<size_t, size_t>((ADDRINT)return_address, count) );
    } else {
      rw_map->at((ADDRINT)return_address) += count;
    }
  }

  start_count(threadid);
}

void Task_Profiler::Fini() {
  size_t count = stop_n_get_count(0);

  for (unsigned int threadid = 0; threadid < NUM_THREADS; threadid++) {
    for (unsigned int i = 0; i < last_allocated[threadid]; i++) {
      report[threadid] << step_work_list[threadid][i].step_parent << ","
		       << step_work_list[threadid][i].work;
      if (step_work_list[threadid][i].region_work != NULL) {
	for (std::map<size_t, size_t>::iterator it=step_work_list[threadid][i].region_work->begin();
	     it!=step_work_list[threadid][i].region_work->end(); ++it) {
	  report[threadid] << "," << it->first << "," << it->second;
	}
      }
      report[threadid] << std::endl;
    }
  }

  size_t cur_step = taskGraph->getCurStep(0);
  report[0] << cur_step << ","
	    << step_work_list[0][last_allocated[0]].work + count;
  if (step_work_list[0][last_allocated[0]].region_work != NULL) {
    for (std::map<size_t, size_t>::iterator it=step_work_list[0][last_allocated[0]].region_work->begin();
	 it!=step_work_list[0][last_allocated[0]].region_work->end(); ++it) {
      report[0] << "," << it->first << "," << it->second;
    }
  }
  report[0] << std::endl;

  for (unsigned int i = 0; i < NUM_THREADS; i++) {
    report[i].close();
  }

  //write callsite info to file
  taskGraph->dumpCallsiteInfo();

  if (!regionMap.empty()) {
    dumpRegionInfo();
  }
}

void Task_Profiler::dumpRegionInfo() {
  std::ofstream report_region_info;
  report_region_info.open("region_info.csv");
  for (std::map<size_t,struct CallSiteData*>::iterator it=regionMap.begin();
	 it!=regionMap.end(); ++it) {
    struct CallSiteData* cs_data = it->second;
    report_region_info << it->first << ","
		       << cs_data->cs_filename << ","
		       << cs_data->cs_line_number
		       << std::endl;
  }
  report_region_info.close();
}
