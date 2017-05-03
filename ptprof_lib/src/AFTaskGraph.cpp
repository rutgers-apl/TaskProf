#include <iostream>
#include <assert.h>
#include "AFTaskGraph.H"

AFTaskGraph::AFTaskGraph() {
  last_allocated_node = 0;
  size_t length = NUM_GRAPH_NODES * sizeof(struct AFTask);
  tgraph_nodes = (struct AFTask*) mmap(0, length, PROT_READ|PROT_WRITE, MMAP_FLAGS, -1, 0);

  size_t head_node = create_node(FINISH, 0);
  cur_dpst_index[0].push(head_node);
  tgraph_nodes[head_node].sp_root_n_wt_flag = true;

  size_t newStep_index = create_node(STEP, 0);
  struct AFTask* newStep = &tgraph_nodes[newStep_index];
  newStep->parent = head_node;
    
  tgraph_nodes[head_node].num_children++;
  tgraph_nodes[head_node].cur_step = newStep_index;

}

size_t AFTaskGraph::create_node(NodeType node_type, size_t task_id){

  last_allocated_node++;
  int index = last_allocated_node;
  initialize_task(index, node_type, task_id); 

  return index;
}

void AFTaskGraph::initialize_task (size_t index,
			NodeType node_type, 
			size_t val){

  tgraph_nodes[index].cur_step = 0;
  tgraph_nodes[index].parent = 0;
  tgraph_nodes[index].taskId = val;
  tgraph_nodes[index].type = node_type;

  tgraph_nodes[index].num_children = 0;
  tgraph_nodes[index].young_ns_child = NO_TYPE;

  tgraph_nodes[index].call_site = 0;

  tgraph_nodes[index].sp_root_n_wt_flag = false;
}

struct AFTask* AFTaskGraph::getCurStep(THREADID threadid) {
  return &tgraph_nodes[tgraph_nodes[cur_dpst_index[threadid].top()].cur_step];
}

struct AFTask* AFTaskGraph::getCurTask(THREADID threadid) {
  return &tgraph_nodes[cur_dpst_index[threadid].top()];
}

struct AFTask* AFTaskGraph::getCurParent(THREADID threadid) {
  struct AFTask* cur_node = &tgraph_nodes[cur_dpst_index[threadid].top()];
  return &tgraph_nodes[cur_node->parent];
}

bool AFTaskGraph::checkForStep (struct AFTask* cur_node) {
  return (cur_node->young_ns_child == ASYNC);
}

void AFTaskGraph::CaptureSpawnOnly(THREADID threadid, size_t taskId, void* return_address)
{
  PIN_GetLock(&lock, 0);

  struct AFTask* cur_node = &tgraph_nodes[cur_dpst_index[threadid].top()];

  //if current node rightmost child is Async or not
  //check for STEP and prev of STEP == ASYNC
  struct AFTask* newNode;
  size_t newNode_index;
  if (checkForStep(cur_node)) {
    newNode_index = create_node(ASYNC, taskId);
    newNode = &tgraph_nodes[newNode_index];
    newNode->parent = cur_dpst_index[threadid].top();

    newNode->call_site = (ADDRINT)return_address;

    cur_node->num_children++;
    cur_node->young_ns_child = ASYNC;
    
  } else {
    size_t newFinish_index = create_node(FINISH, cur_node->taskId);
    struct AFTask* newFinish = &tgraph_nodes[newFinish_index];
    newFinish->parent = cur_dpst_index[threadid].top();

    cur_node->num_children++;
    cur_node->young_ns_child = FINISH;

    newNode_index = create_node(ASYNC, taskId);
    newNode = &tgraph_nodes[newNode_index];
    newNode->parent = newFinish_index;

    newNode->call_site = (ADDRINT)return_address;

    newFinish->num_children++;
    newFinish->young_ns_child = ASYNC;

    cur_dpst_index[threadid].push(newFinish_index);
    cur_node = newFinish;

  }

  // add newNode->children STEP and cur_node->children STEP
  size_t newStep_index = create_node(STEP, newNode->taskId);
  struct AFTask* newStep = &tgraph_nodes[newStep_index];
  newStep->parent = newNode_index;

  newNode->num_children++;
  newNode->cur_step = newStep_index;

  newStep_index = create_node(STEP, cur_node->taskId);
  newStep = &tgraph_nodes[newStep_index];
  newStep->parent = cur_dpst_index[threadid].top();

  cur_node->num_children++;
  cur_node->cur_step = newStep_index;

  temp_cur_map.insert(std::pair<size_t, size_t>(taskId, newNode_index));

  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureExecute(THREADID threadid, size_t taskId)
{
  PIN_GetLock(&lock, 0);
  size_t temp_cur = temp_cur_map.at(taskId);
  temp_cur_map.erase(taskId);
  cur_dpst_index[threadid].push(temp_cur);
  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureSpawnAndWait(THREADID threadid, size_t taskId, void* return_address)
{
  PIN_GetLock(&lock, 0);

  size_t cur_index = cur_dpst_index[threadid].top();
  struct AFTask* cur_node = &tgraph_nodes[cur_index];
  size_t newFinish_index = create_node(FINISH, taskId);
  struct AFTask* newFinish = &tgraph_nodes[newFinish_index];
  newFinish->parent = cur_index;

  newFinish->call_site = (ADDRINT)return_address;

  newFinish->sp_root_n_wt_flag = true;

  cur_node->num_children++;
  cur_node->young_ns_child = FINISH;

  size_t newStep_index = create_node(STEP, newFinish->taskId);
  struct AFTask* newStep = &tgraph_nodes[newStep_index];
  newStep->parent = newFinish_index;

  newFinish->num_children++;
  newFinish->cur_step = newStep_index;

  temp_cur_map.insert(std::pair<size_t, size_t>(taskId,newFinish_index));
  
  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureReturn(THREADID threadid)
{
  //PIN_GetLock(&lock, 0);
  size_t cur_index = cur_dpst_index[threadid].top();
  struct AFTask* cur_node = &tgraph_nodes[cur_index];
  if (cur_node->type == FINISH && cur_node->sp_root_n_wt_flag == false) {
    cur_dpst_index[threadid].pop();
  }
  cur_dpst_index[threadid].pop();
  //PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureWait(THREADID threadid)
{
  PIN_GetLock(&lock, 0);
  
  size_t cur_index = cur_dpst_index[threadid].top();
  struct AFTask* cur_node = &tgraph_nodes[cur_index];
  size_t newStep_index = create_node(STEP, cur_node->taskId);
  struct AFTask* newStep = &tgraph_nodes[newStep_index];

  newStep->parent = cur_index;

  cur_node->num_children++;
  cur_node->cur_step = newStep_index;

  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureWaitOnly(THREADID threadid)
{
  PIN_GetLock(&lock, 0);
  size_t cur_index = cur_dpst_index[threadid].top();
  struct AFTask* cur_node = &tgraph_nodes[cur_index];

  cur_dpst_index[threadid].pop();

  cur_index = cur_dpst_index[threadid].top();

  cur_node = &tgraph_nodes[cur_index];

  size_t newStep_index = create_node(STEP, cur_node->taskId);
  struct AFTask* newStep = &tgraph_nodes[newStep_index];

  newStep->parent = cur_index;

  cur_node->num_children++;
  cur_node->cur_step = newStep_index;
  
  PIN_ReleaseLock(&lock);
}

void AFTaskGraph::CaptureSetTaskId(THREADID threadid, size_t taskId,
				   int sp_only, void* return_address, 
				   const char* file, int line, bool par_for)
{

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

struct CallSiteData* AFTaskGraph::getSourceFileAndLine(size_t return_address) {
  return callSiteMap.at(return_address);
}
