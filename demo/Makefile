# ifneq (,$(shell which clang++ 2>/dev/null))
# CXX=clang++
# CXXFLAGS = -I$(TP_ROOT)/include -I$(TBBROOT)/include -ftaskdebug -g
# else
CXX=g++
CXXFLAGS = -I$(TP_ROOT)/include -I$(TBBROOT)/include -g -std=c++11
# endif # clang++

#LIBS = -L$(TBBROOT)/obj -ltbb -L$(TP_ROOT)/obj -ltprof
LIBS = -ltbb -L$(TP_ROOT)/obj -ltprof -lpthread

SRCS = $(wildcard *.cpp)

PROGS = $(patsubst %.cpp,%,$(SRCS))

all: $(PROGS)

%: %.cpp
	@$(CXX) $(CXXFLAGS) -o $@ $< $(LIBS)
	@echo "Compiled "$<" successfully!"

clean:
	rm -rf $(PROGS) *~ *.out
