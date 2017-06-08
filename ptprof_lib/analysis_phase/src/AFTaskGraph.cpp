#include <iostream>
#include <assert.h>
#include <string>
#include <sstream>
#include <fstream>

#include "AFTaskGraph.H"

AFTaskGraph::AFTaskGraph() {
  last_allocated_node = 0;
  size_t length = NUM_GRAPH_NODES * sizeof(struct AFTask);
  tgraph_nodes = (struct AFTask*) mmap(0, length, PROT_READ|PROT_WRITE, MMAP_FLAGS, -1, 0);

  create_dpst();
}

size_t AFTaskGraph::create_node(NodeType node_type, size_t call_site, size_t parent_idx) {
  last_allocated_node++;
  size_t index = last_allocated_node;
  initialize_task(index, node_type, call_site, parent_idx);
  return index;
}

void AFTaskGraph::initialize_task (size_t index, NodeType node_type, size_t call_site, size_t parent_idx){
  tgraph_nodes[index].parent = parent_idx;
  tgraph_nodes[index].type = node_type;
  tgraph_nodes[index].num_children = 0;
  tgraph_nodes[index].call_site = call_site;

  //tgraph_nodes[index].cur_step = 0;
  //tgraph_nodes[index].taskId = val;
  //tgraph_nodes[index].young_ns_child = NO_TYPE;
  //tgraph_nodes[index].sp_root_n_wt_flag = false;
  //tgraph_nodes[index].ut_id.depth = 0;
  //tgraph_nodes[index].ut_id.sum_seq_nos = 0;
}

void AFTaskGraph::create_dpst() {
  std::ifstream report_int_nodes, step_work_file;

  /* First create all internal async and finish nodes */
  report_int_nodes.open("int_nodes.csv");
  if (report_int_nodes.is_open()) {
    std::string line;
    while (std::getline (report_int_nodes, line)) {
      std::stringstream sstream(line);
      std::string parent_idx;
      std::getline(sstream, parent_idx, ',');
      size_t parent_index = std::stoul(parent_idx);
      std::string node_type;
      std::getline(sstream, node_type, ',');
      std::string call_site;
      std::getline(sstream, call_site, ',');
      //std::cout << "Info: " << seq_nos_str << "," << node_type << "," << call_site << std::endl;

      create_node(static_cast<NodeType>(std::stoi(node_type)), std::stoul(call_site), parent_index);
      //update the number of children field in the parent index
      if (parent_index != 0) {
	tgraph_nodes[parent_index].num_children++;
      }
    }
    report_int_nodes.close();
  } else {
    std::cerr << "Work data of step nodes not available\n";
    assert(0);
  }

  /* Next create step nodes and add work information too */
  for (unsigned int i = 0; i < NUM_THREADS; i++) {
    /* Read work data of each node collected in collection phase */
    std::string file_name = "step_work_" + std::to_string(i) + ".csv";
    step_work_file.open(file_name);

    if (step_work_file.is_open()) {
      std::string line;
      while (std::getline (step_work_file, line)) {
	//std::cout << line << std::endl;
	std::stringstream sstream(line);

	std::string parent_idx;
	std::getline(sstream, parent_idx, ',');
	size_t parent_index = std::stoul(parent_idx);
	size_t insert_idx = create_node(STEP, 0, parent_index);
	//update the number of children field in the parent index
	if (parent_index != 0) {
	  tgraph_nodes[parent_index].num_children++;
	}

	//update step work
	std::string step_work_str;
	std::getline(sstream, step_work_str, ',');
	tgraph_nodes[insert_idx].t_prof.work = std::stoul(step_work_str);

	//update region work if present
	std::string region;
	while(std::getline(sstream, region, ',')) {
	  std::string reg_work;
	  std::getline(sstream, reg_work, ',');
	  if (tgraph_nodes[insert_idx].t_prof.region_work == NULL) {
	    tgraph_nodes[insert_idx].t_prof.region_work = new std::map<size_t, size_t>();
	    tgraph_nodes[insert_idx].t_prof.region_work->
	      insert( std::pair<size_t, size_t>(std::stoul(region), std::stoul(reg_work)) );
	  } else {
	    std::map<size_t, size_t>* rw_map = tgraph_nodes[insert_idx].t_prof.region_work;
	    if (rw_map->count(std::stoul(region)) == 0) {
	      rw_map->insert( std::pair<size_t, size_t>(std::stoul(region), std::stoul(reg_work)) );
	    } else {
	      rw_map->at(std::stoul(region)) += std::stoul(reg_work);
	    }
	  }
	}
      }
      step_work_file.close();
    } else {
      std::cerr << "Work data of step nodes not available\n";
      assert(0);
    }
  }
}

struct CallSiteData* AFTaskGraph::getSourceFileAndLine(size_t return_address) {
  if (callSiteMap.empty()) {
    std::ifstream call_site_file;
    call_site_file.open("callsite_info.csv");
    if (call_site_file.is_open()) {
      std::string line;      
      while (std::getline (call_site_file, line)) {
	std::stringstream sstream(line);
	std::string cs_str;
	std::getline(sstream, cs_str, ',');
	std::string fn_str;
	std::getline(sstream, fn_str, ',');
	std::string line_str;
	std::getline(sstream, line_str, ',');
	std::string par_for_str;
	std::getline(sstream, par_for_str, ',');
	
	struct CallSiteData* cs_data = new CallSiteData();
	cs_data->cs_filename = fn_str;
	cs_data->cs_line_number = std::stoi(line_str);
	cs_data->par_for = std::stoi(par_for_str);
	
	callSiteMap.insert(std::pair<size_t, struct CallSiteData*>(std::stoul(cs_str), cs_data));
      }
    } else {
      std::cerr << "Work data of step nodes not available\n";
      assert(0);
    }
  }

  return callSiteMap.at(return_address);
}
