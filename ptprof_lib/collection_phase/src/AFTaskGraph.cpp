#include <iostream>
#include <string.h>
//#include <assert.h>
#include "AFTaskGraph.H"

AFTaskGraph::AFTaskGraph() {
  last_allocated_node = 1;

  struct AFTask* head_node = create_node(0);
  cur_dpst_index[0].push(head_node);

  taskLogger->log(0,head_node->node_id,0,0,FINISH,0);

  //Add step node as the first child of Head node
  head_node->child_id = last_allocated_node;
  last_allocated_node++;
}

struct AFTask* AFTaskGraph::create_node(THREADID threadid) {
  //initialize task;
  struct AFTask* new_node = new AFTask();
  new_node->node_id = last_allocated_node;
  last_allocated_node++;
  new_node->young_ns_child = NO_TYPE;
  new_node->sync_finish_flag = false;

  return new_node;
}

size_t AFTaskGraph::getCurStepParent(THREADID threadid) {
  return (cur_dpst_index[threadid].top())->node_id;
}

size_t AFTaskGraph::getCurStepId(THREADID threadid) {
  return (cur_dpst_index[threadid].top())->child_id;
}

void AFTaskGraph::CaptureSpawnOnly(THREADID threadid, size_t taskId, void* return_address)
{
  PIN_GetLock(&lock, 0);

  struct AFTask* cur_node = cur_dpst_index[threadid].top();

  //if current node rightmost child is Async or not
  //check for STEP and prev of STEP == ASYNC
  struct AFTask* newNode;
  if (cur_node->young_ns_child == ASYNC) {
    newNode = create_node(threadid);
    cur_node->young_ns_child = ASYNC;

    taskLogger->log(threadid,newNode->node_id,cur_node->node_id,0,ASYNC,(ADDRINT)return_address);
  } else {
    struct AFTask* newFinish = create_node(threadid);
    cur_node->young_ns_child = FINISH;

    newFinish->sync_finish_flag = true;

    taskLogger->log(threadid,newFinish->node_id,cur_node->node_id,0,FINISH,0);

    newNode = create_node(threadid);
    newFinish->young_ns_child = ASYNC;

    taskLogger->log(threadid,newNode->node_id,newFinish->node_id,0,ASYNC,(ADDRINT)return_address);

    cur_dpst_index[threadid].push(newFinish);
    cur_node = newFinish;
  }

  newNode->child_id = last_allocated_node;
  last_allocated_node++;

  cur_node->child_id = last_allocated_node;
  last_allocated_node++;  

  temp_cur_map.insert(std::pair<size_t, struct AFTask*>(taskId, newNode));

  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureSpawnAndWaitForAll(THREADID threadid, size_t taskId, void* return_address)
{
  PIN_GetLock(&lock, 0);

  struct AFTask* cur_node = cur_dpst_index[threadid].top();

  //if current node rightmost child is Async or not
  //check for STEP and prev of STEP == ASYNC
  struct AFTask* newNode;
  if (cur_node->young_ns_child == ASYNC) {
    newNode = create_node(threadid);
    cur_node->young_ns_child = ASYNC;

    taskLogger->log(threadid,newNode->node_id,cur_node->node_id,0,ASYNC,(ADDRINT)return_address);
  } else {
    struct AFTask* newFinish = create_node(threadid);
    cur_node->young_ns_child = FINISH;

    newFinish->sync_finish_flag = true;

    taskLogger->log(threadid,newFinish->node_id,cur_node->node_id,0,FINISH,0);

    newNode = create_node(threadid);
    newFinish->young_ns_child = ASYNC;

    taskLogger->log(threadid,newNode->node_id,newFinish->node_id,0,ASYNC,(ADDRINT)return_address);
    cur_dpst_index[threadid].push(newFinish);
    cur_node = newFinish;
  }

  newNode->child_id = last_allocated_node;
  last_allocated_node++;
  temp_cur_map.insert(std::pair<size_t, struct AFTask*>(taskId, newNode));

  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureExecute(THREADID threadid, size_t taskId)
{
  PIN_GetLock(&lock, 0);
  struct AFTask* temp_cur = temp_cur_map.at(taskId);
  temp_cur_map.erase(taskId);
  cur_dpst_index[threadid].push(temp_cur);
  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureSpawnAndWait(THREADID threadid, size_t taskId, void* return_address)
{
  PIN_GetLock(&lock, 0);

  struct AFTask* cur_node = cur_dpst_index[threadid].top();
  struct AFTask* newFinish = create_node(threadid);
  cur_node->young_ns_child = FINISH;

  taskLogger->log(threadid,newFinish->node_id,cur_node->node_id,0,FINISH,(ADDRINT)return_address);  
  newFinish->child_id = last_allocated_node;
  last_allocated_node++;
  temp_cur_map.insert(std::pair<size_t, struct AFTask*>(taskId,newFinish));
  
  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureReturn(THREADID threadid)
{
  //PIN_GetLock(&lock, 0);
  struct AFTask* cur_node = cur_dpst_index[threadid].top();
  if (cur_node->sync_finish_flag == true) {
    cur_dpst_index[threadid].pop();      
    delete cur_node;
    cur_node = cur_dpst_index[threadid].top();
  }
  cur_dpst_index[threadid].pop();
  delete cur_node;
  //PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureWait(THREADID threadid)
{
  PIN_GetLock(&lock, 0);
  
  struct AFTask* cur_node = cur_dpst_index[threadid].top();

  cur_node->child_id = last_allocated_node;
  last_allocated_node++;

  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureWaitOnly(THREADID threadid)
{
  PIN_GetLock(&lock, 0);
  struct AFTask* cur_node = cur_dpst_index[threadid].top();

  cur_dpst_index[threadid].pop();
  delete cur_node;

  cur_node = cur_dpst_index[threadid].top();

  cur_node->child_id = last_allocated_node;
  last_allocated_node++;  
  
  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureSetTaskId(THREADID threadid, size_t taskId,
				   int sp_only, void* return_address, 
				   const char* file, int line, bool par_for) {
  if (line == 0 || file == NULL) {
    return_address = 0;
  } else {
    if (callSiteMap.count((ADDRINT)return_address) == 0) {
      struct CallSiteData* callsiteData = new CallSiteData();
      callsiteData->cs_filename = file;
      callsiteData->cs_line_number = line;
      callsiteData->par_for = par_for;
      callSiteMap.insert(std::pair<size_t, struct CallSiteData*>((ADDRINT)return_address, callsiteData));
    }
  }

  if (sp_only) {
    CaptureSpawnOnly(threadid, taskId, return_address);
  } else {
    CaptureSpawnAndWait(threadid, taskId, return_address);
  }
}

void AFTaskGraph::CaptureSetTaskId(THREADID threadid, size_t taskId,
				   void* return_address, 
				   const char* file, int line, bool par_for) {
  if (line == 0 || file == NULL) {
    return_address = 0;
  } else {
    if (callSiteMap.count((ADDRINT)return_address) == 0) {
      struct CallSiteData* callsiteData = new CallSiteData();
      callsiteData->cs_filename = file;
      callsiteData->cs_line_number = line;
      callsiteData->par_for = par_for;
      callSiteMap.insert(std::pair<size_t, struct CallSiteData*>((ADDRINT)return_address, callsiteData));
    }
  }

  CaptureSpawnAndWaitForAll(threadid, taskId, return_address);
}

void AFTaskGraph::dumpCallsiteInfo() {
  std::ofstream report_callsite_info;
  report_callsite_info.open("callsite_info.csv");
  for (std::unordered_map<size_t,struct CallSiteData*>::iterator it=callSiteMap.begin();
	 it!=callSiteMap.end(); ++it) {
    struct CallSiteData* cs_data = it->second;
    report_callsite_info << it->first << ","
			 << cs_data->cs_filename << ","
			 << cs_data->cs_line_number << ","
			 << cs_data->par_for
			 << std::endl;
  }
  report_callsite_info.close();
}
