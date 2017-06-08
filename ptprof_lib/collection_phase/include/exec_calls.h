#ifndef EXEC_CALLS_H
#define EXEC_CALLS_H

#pragma GCC push_options
#pragma GCC optimize ("O0")

#include "tbb/atomic.h"
#include "tbb/tbb_thread.h"

#include "AFTaskGraph.H"
#include "Task_Profiler.H"

typedef tbb::internal::tbb_thread_v3::id TBB_TID;

extern std::map<TBB_TID, size_t> tid_map;

//extern PIN_LOCK tid_map_lock;

extern tbb::atomic<size_t> task_id_ctr;

extern tbb::atomic<size_t> tid_ctr;

extern AFTaskGraph* taskGraph;

extern Task_Profiler* taskProf;

#define __OPTIMIZE__BEGIN__ __optimize_begin__(__FILE__, __LINE__);
#define __OPTIMIZE__END__ __optimize_end__(__FILE__, __LINE__);
//#define __OPTIMIZE__BEGIN__
//#define __OPTIMIZE__END__

extern "C" {
  __attribute__((noinline)) void TD_Activate();

  __attribute__((noinline)) void Fini();

  __attribute__((noinline)) void __exec_begin__(unsigned long taskId);
  
  __attribute__((noinline)) void __exec_end__(unsigned long taskId);

  __attribute__((noinline)) void __optimize_begin__(const char* file, int line);

  __attribute__((noinline)) void __optimize_end__(const char* file, int line);

  size_t get_cur_tid( );
}

#pragma GCC pop_options

#endif
