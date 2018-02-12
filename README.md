TaskProf is an tool for identifying performance bottlenecks in task-based parallel programs written in C++ using Intel Threading Building Blocks library. For details on how TaskProf works, please refer to our [paper](https://dl.acm.org/citation.cfm?doid=3106237.3106254).

## Requirements

TaskProf relies on hardware support for performance counters. To check if the machine supports hardware performance counters,

	dmesg | grep PMU
	
If the output is "Performance Events: Unsupported...", then the machine does not support performance counters and TaskProf cannot be executed on the machine.

## Installation

Install perf. On Ubuntu the following command can be used to install perf.

	sudo apt-get install linux-tools-common linux-tools-generic linux-tools-`uname -r`
	
Install TaskProf.

	source build.sh
	
## Usage

We demonstrate how to use TaskProf on the tree sum test program under the tests directory.
To execute TaskProf on the tree sum test program:

	cd <path_to_TaskProf>/tests/treesum
	make
	./treesum
	
This generates the profile data from the execution of the program. Next execute TaskProf's post-processing program to generate the profile.

	$TP_GENPROF/gentprof
	
Note, TP_GENPROF is an environment variable already setup by the build script.

This generates the parallelism profile in the file ws_profile.csv. The first row of the profile describes what each column represents. The second row specifies the parallelism of the entire program and the percentage of work performed on the critical path by the main function. The rest of the rows specify the parallelism and the critical path percentage for each static spawn site in the program.
