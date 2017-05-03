#!/bin/bash
cwd="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )" #
export TP_ROOT="$cwd" #
tp_obj="$cwd/obj" #
if [ -z "$CPATH" ]; then #
    export CPATH="${TP_ROOT}/include" #
else #
    export CPATH="${TP_ROOT}/include:$CPATH" #
fi #
if [ -z "$LIBRARY_PATH" ]; then #
    export LIBRARY_PATH="${tp_obj}" #
else #
    export LIBRARY_PATH="${tp_obj}:$LIBRARY_PATH" #
fi #
if [ -z "$LD_LIBRARY_PATH" ]; then #
    export LD_LIBRARY_PATH="${tp_obj}" #
else #
    export LD_LIBRARY_PATH="${tp_obj}:$LD_LIBRARY_PATH" #
fi #
 #
