#include "t_debug_task.h"
#include "tbb/task_scheduler_init.h"
#include <fstream>
#include <iostream>
#include <assert.h>

#define NUM_THREADS_HERE 4

size_t grain_size;
size_t total_elems;

size_t** primes;
size_t* num_primes;

bool IsPrime(size_t num) {
  if (num <= 1) return false; // zero and one are not prime
  for (size_t i=2; i*i<=num; i++) {
    if (num % i == 0) return false;
  }
  return true;
}

class CheckPrimesTask:public tbb::t_debug_task {
private:
  size_t begin;
  size_t end;

public:
  CheckPrimesTask(size_t begin, size_t end): begin(begin), end(end){}
  
  tbb::task* execute() {
    __exec_begin__(getTaskId(), __FILE__, __LINE__);
    if ((end-begin) <= grain_size) {
      //__OPTIMIZE__BEGIN__
      //determine range_id
      size_t range_id;
      if (begin == 0) {
        range_id = 0;
      } else {
        range_id = begin/grain_size;
      }
      for (size_t i = begin; i < end; i++) {
      	if (IsPrime(i)) {
      	  primes[range_id][num_primes[range_id]] = i;
      	  num_primes[range_id]++;
      	}
      }
      //__OPTIMIZE__END__
    } else {      
      set_ref_count(3);
      tbb::task* a = new( tbb::task::allocate_child() ) CheckPrimesTask(begin, begin + ((end-begin)/2));
      tbb::t_debug_task::spawn(*a, __FILE__, __LINE__);
      tbb::task* b = new( tbb::task::allocate_child() ) CheckPrimesTask(begin + ((end-begin)/2), end);
      tbb::t_debug_task::spawn(*b, __FILE__, __LINE__);
      tbb::t_debug_task::wait_for_all(__FILE__, __LINE__);
    }
    __exec_end__(getTaskId(), __FILE__, __LINE__);
    return NULL;
  }
};

int main(int argc, char* argv[]) {
  TD_Activate(__FILE__, __LINE__);
  if (argc != 4) {
    std::cout << "Format: ./detect_primes <range_begin> <range_end> <grain_size>" << std::endl;
    return 0;
  }
  
  size_t range_begin = strtoul(argv[1], NULL, 0);
  size_t range_end = strtoul(argv[2], NULL, 0);
  assert(range_end > range_begin);
  grain_size = strtoul(argv[3], NULL, 0);
  
  tbb::task_scheduler_init init(NUM_THREADS_HERE);
  total_elems = range_end - range_begin;
  assert(grain_size <= total_elems);

  size_t ranges = total_elems/grain_size;
  primes = new size_t*[ranges];
  num_primes = new size_t[ranges];
  for (size_t i = 0; i < ranges; i++) {
    num_primes[i] = 0;
    primes[i] = new size_t[total_elems/ranges];
  }

  tbb::task* a = new( tbb::task::allocate_root() ) CheckPrimesTask(range_begin, range_end);
  tbb::t_debug_task::spawn_root_and_wait(*a, __FILE__, __LINE__);
  
  std::ofstream report;
  report.open("primes.txt");
  for (unsigned int i = 0; i < ranges; i++) {
    for (size_t j = 0; j < num_primes[i]; j++)
      report << primes[i][j] << "   ";
  }

  Fini(__FILE__, __LINE__);
}
