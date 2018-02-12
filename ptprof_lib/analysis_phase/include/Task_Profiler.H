#ifndef TASK_PROFILER_H
#define TASK_PROFILER_H

#include <string>
#include <unordered_map>

#include "AFTaskGraph.H"
#include "WorkSpanData.H"

class Task_Profiler {
private:
  AFTaskGraph* taskGraph;
  std::unordered_map<size_t, struct CallSiteData*> regionMap;
  std::unordered_map<size_t, struct WorkSpanData>* workSpanMap;

  bool recursiveCall(struct AFTask* node);
  void checkUpdateParentWorkSpan(size_t parent_index, size_t* num_processed, std::unordered_map<size_t, struct WorkSpanData>* workSpanMap);
  void calculateRecurse(struct AFTask* node, std::unordered_map<size_t, struct WorkSpanData>* workSpanMap);
  void calculateWorkSpan(struct AFTask* node);
  bool checkRecursiveCall(struct AFTask* node, size_t call_site);

  /********** INCREASE PARALLELISM OF EACH STATIC REGION **************/
  void report_region_increase_par();
  void checkUpdateParentWorkSpan_region(size_t parent_index, size_t* num_processed, size_t updated_region, unsigned int par_increase);
  void calculateRecurse_region(struct AFTask* node, size_t updated_region, unsigned int par_increase);
  void calculateWorkSpan_region(struct AFTask* node, size_t updated_region, unsigned int par_increase);
  void reset_work_span();
  /***************************************************************/


  /********** REPORT %AGE OF CRITICAL PATH LENGTH **************/
  void merge_critical_call_sites(std::unordered_map<size_t, size_t>*, std::unordered_map<size_t, size_t>*);
  void insert_cs_data(std::unordered_map<size_t, size_t>*, size_t call_site, size_t local_local_work);
  /***************************************************************/

public:
  Task_Profiler();
  size_t GenerateProfile(std::string);
};

#endif
