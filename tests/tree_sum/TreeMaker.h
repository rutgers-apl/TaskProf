/*
  Copyright 2005-2014 Intel Corporation.  All Rights Reserved.

  This file is part of Threading Building Blocks. Threading Building Blocks is free software;
  you can redistribute it and/or modify it under the terms of the GNU General Public License
  version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
  distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See  the GNU General Public License for more details.   You should have received a copy of
  the  GNU General Public License along with Threading Building Blocks; if not, write to the
  Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

  As a special exception,  you may use this file  as part of a free software library without
  restriction.  Specifically,  if other files instantiate templates  or use macros or inline
  functions from this file, or you compile this file and link it with other files to produce
  an executable,  this file does not by itself cause the resulting executable to be covered
  by the GNU General Public License. This exception does not however invalidate any other
  reasons why the executable file might be covered by the GNU General Public License.
*/

#ifndef TREE_MAKER_H_
#define TREE_MAKER_H_

#include "t_debug_task.h"

static double Pi = 3.14159265358979;

const bool tbbmalloc = true;
const bool stdmalloc = false;

template<bool use_tbbmalloc>
class TreeMaker {

  class SubTreeCreationTask: public tbb::t_debug_task {
    TreeNode*& my_root;
    typedef TreeMaker<use_tbbmalloc> MyTreeMaker;

  public:
  SubTreeCreationTask( TreeNode*& root, long number_of_nodes ) : my_root(root) {
      my_root = MyTreeMaker::allocate_node();
      my_root->node_count = number_of_nodes;
      my_root->value = Value(Pi*number_of_nodes);
    }

    tbb::task* execute() {
      __exec_begin__(getTaskId(),__FILE__, __LINE__);
      long subtree_size = my_root->node_count - 1;
      if( subtree_size < GRANULARITY ) { /* grainsize */
	my_root->left  = MyTreeMaker::do_all_1(subtree_size/2);
	my_root->right = MyTreeMaker::do_all_1(subtree_size - subtree_size/2);
      } else {
	// Create tasks before spawning any of them.
	tbb::t_debug_task* a = new( allocate_child() ) SubTreeCreationTask(my_root->left,subtree_size/2);
	tbb::t_debug_task* b = new( allocate_child() ) SubTreeCreationTask(my_root->right,subtree_size - subtree_size/2);
	set_ref_count(3);
	tbb::t_debug_task::spawn(*a, __FILE__, __LINE__);
	tbb::t_debug_task::spawn(*b, __FILE__, __LINE__);
	tbb::t_debug_task::wait_for_all(__FILE__, __LINE__);
      }
      __exec_end__(getTaskId(),__FILE__, __LINE__);
      return NULL;
    }
  };

 public:
  static TreeNode* allocate_node() {
    return use_tbbmalloc? tbb::scalable_allocator<TreeNode>().allocate(1) : new TreeNode;
  }

  static TreeNode* do_all_1( long number_of_nodes ) {
    if( number_of_nodes==0 ) {
      return NULL;
    } else {
      TreeNode* n = allocate_node();
      n->node_count = number_of_nodes;
      n->value = Value(Pi*number_of_nodes);
      --number_of_nodes;
      n->left  = do_all_1( number_of_nodes/2 ); 
      n->right = do_all_1( number_of_nodes - number_of_nodes/2 );
      return n;
    }
  }

  static TreeNode* do_all_2( long number_of_nodes ) {
    TreeNode* root_node;
    SubTreeCreationTask& a = *new(tbb::task::allocate_root()) SubTreeCreationTask(root_node, number_of_nodes);
    tbb::t_debug_task::spawn_root_and_wait(a, __FILE__, __LINE__);
    return root_node;
  }

  static TreeNode* create_and_time( long number_of_nodes, bool silent=false ) {
    TreeNode* root = allocate_node();
    root->node_count = number_of_nodes;
    root->value = Value(Pi*number_of_nodes);
    --number_of_nodes;

    __OPTIMIZE__BEGIN__
    root->left  = do_all_1( number_of_nodes/2 );
    __OPTIMIZE__END__
    root->right = do_all_2( number_of_nodes - number_of_nodes/2 );

    return root;
  }
};

#endif // TREE_MAKER_H_
