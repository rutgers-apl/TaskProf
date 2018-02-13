#ifndef __TBB_t_debug_task_H
#define __TBB_t_debug_task_H

#pragma GCC push_options
#pragma GCC optimize ("O0")

#include <map>
#include <iostream>
#include "tbb/task.h"
#include "exec_calls.h"

namespace tbb {

  class t_debug_task: public task {
  private:
    size_t taskId;
    void setTaskId(size_t taskId, int sp_only) { this->taskId = taskId; }

  public:
    static void spawn( task& t, const char* file, int line, bool par_for = false );__attribute__((optimize(0)));
    static void spawn_root_and_wait( task& root, const char* file, int line, void* ret_address = 0, bool par_for = false );__attribute__((optimize(0)));
    void spawn_and_wait_for_all( task& child, const char* file, int line );__attribute__((optimize(0)));
    void wait_for_all( );__attribute__((optimize(0)));
    size_t getTaskId( ) { return taskId; }
  };

  inline void t_debug_task::spawn(task& t, const char* file, int line, bool par_for) {
    /*PROF CALL*/taskProf->TP_CaptureSpawn_Entry(get_cur_tid());
    
    (static_cast<t_debug_task&>(t)).setTaskId(++task_id_ctr, 1);
    if (par_for) {
      taskGraph->CaptureSetTaskId(get_cur_tid(), (static_cast<t_debug_task&>(t)).getTaskId(), true, __builtin_return_address(0), NULL, 0);
    } else {
      taskGraph->CaptureSetTaskId(get_cur_tid(), (static_cast<t_debug_task&>(t)).getTaskId(), true, __builtin_return_address(0), file, line);
    }

    task::spawn(t);

    /*PROF CALL*/taskProf->TP_CaptureSpawn_Exit(get_cur_tid());
  }

  inline void t_debug_task::spawn_root_and_wait( task& root, const char* file, int line, void* ret_address, bool par_for ) {
    /*PROF CALL*/taskProf->TP_CaptureSpawn_Entry(get_cur_tid());

    (static_cast<t_debug_task&>(root)).setTaskId(++task_id_ctr, 0);
    if (ret_address == 0) {
      taskGraph->CaptureSetTaskId(get_cur_tid(), (static_cast<t_debug_task&>(root)).getTaskId(), false, __builtin_return_address(0), file, line, par_for);
    } else {
      taskGraph->CaptureSetTaskId(get_cur_tid(), (static_cast<t_debug_task&>(root)).getTaskId(), false, ret_address, file, line, par_for);
    }

    task::spawn_root_and_wait(root);

    taskGraph->CaptureWait(get_cur_tid());

    /*PROF CALL*/taskProf->TP_CaptureSpawn_Exit(get_cur_tid());
  }

  inline void t_debug_task::spawn_and_wait_for_all( task& child,const char* file, int line ) {
    /*PROF CALL*/taskProf->TP_CaptureWait_Entry(get_cur_tid());

    (static_cast<t_debug_task&>(child)).setTaskId(++task_id_ctr, 1);
    taskGraph->CaptureSetTaskId(get_cur_tid(), (static_cast<t_debug_task&>(child)).getTaskId(), __builtin_return_address(0), file, line);

    task::spawn_and_wait_for_all(child);

    taskGraph->CaptureWaitOnly(get_cur_tid());

    /*PROF CALL*/taskProf->TP_CaptureWait_Exit(get_cur_tid());
  }

  inline void t_debug_task::wait_for_all() {    
    /*PROF CALL*/taskProf->TP_CaptureWait_Entry(get_cur_tid());

    task::wait_for_all();

    taskGraph->CaptureWaitOnly(get_cur_tid());

    /*PROF CALL*/taskProf->TP_CaptureWait_Exit(get_cur_tid());
  }

}

#pragma GCC pop_options
#endif
