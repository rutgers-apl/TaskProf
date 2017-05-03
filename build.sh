#!/bin/bash

root_cwd="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )" #
echo $root_cwd

echo "***** Building TaskProf *****"
export LD_LIBRARY_PATH=""
cd $root_cwd
cd ptprof_lib
make clean
make
source tpvars.sh

echo "***** Building TBB *****"
cd $root_cwd
cd tprof-tbb-lib
make clean
make
source obj/tbbvars.sh
cd $root_cwd
