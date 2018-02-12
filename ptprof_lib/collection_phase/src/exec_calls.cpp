#pragma GCC push_options
#pragma GCC optimize ("O0")

#include <map>

#include "exec_calls.h"
#include "Common.H"
#include "t_debug_task.h"

tbb::atomic<size_t> task_id_ctr;
tbb::atomic<size_t> tid_ctr;
PIN_LOCK lock;
PIN_LOCK tid_map_lock;
std::map<TBB_TID, size_t> tid_map;

AFTaskGraph* taskGraph;
Task_Profiler* taskProf;
Task_Logger* taskLogger;

extern "C" {
  void TD_Activate(const char* file, int line) {
    taskLogger = new Task_Logger();
    taskGraph = new AFTaskGraph();
    taskGraph->setStepRegion(0, file, line, true);
    taskProf = new Task_Profiler();
  }

  void Fini(const char* file, int line)
  {
    taskGraph->setStepRegion(0, file, line, false);
    taskProf->Fini();
    //taskGraph->Fini();
  }

  __attribute__((noinline)) void __exec_begin__(unsigned long taskId, const char* file, int line){
    taskGraph->CaptureExecute(get_cur_tid(), taskId);
    
    taskGraph->setStepRegion(get_cur_tid(), file, line, true);
    /*PROF CALL*/taskProf->TP_CaptureExecute(get_cur_tid());
  }

  __attribute__((noinline)) void __exec_end__(unsigned long taskId, const char* file, int line){
    /*PROF CALL*/taskProf->TP_CaptureReturn(get_cur_tid());
    
    taskGraph->setStepRegion(get_cur_tid(), file, line, false);
    taskProf->update_step_and_region(get_cur_tid());
    
    taskGraph->CaptureReturn(get_cur_tid());

  }

  __attribute__((noinline)) void __optimize_begin__(const char* file, int line){
    /*PROF CALL*/taskProf->TP_CaptureBeginOptimize(get_cur_tid(), file, line, __builtin_return_address(0));
  }
  
  __attribute__((noinline)) void __optimize_end__(const char* file, int line){
    /*PROF CALL*/taskProf->TP_CaptureEndOptimize(get_cur_tid(), file, line, __builtin_return_address(0));
  }

  __attribute__((noinline))  size_t get_cur_tid() {
    TBB_TID pthd_id = tbb::this_tbb_thread::get_id();
    size_t my_tid;
    PIN_GetLock(&tid_map_lock, 0);
    if (tid_map.count(pthd_id) == 0) {
      my_tid = tid_ctr++;
      tid_map.insert(std::pair<TBB_TID, size_t>(pthd_id, my_tid));
      PIN_ReleaseLock(&tid_map_lock);
    } else {
      PIN_ReleaseLock(&tid_map_lock);
      my_tid = tid_map.at(pthd_id);
    }

    return my_tid;
  }
}


#pragma GCC pop_options
