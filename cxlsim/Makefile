# Ramulator Variables
RAMULATOR_ROOT := ./ramulator
RAMULATOR_SRCDIR := $(RAMULATOR_ROOT)/src
RAMULATOR_OBJDIR := $(RAMULATOR_ROOT)/obj
RAMULATOR_MAIN := $(RAMULATOR_SRCDIR)/Main.cpp
RAMULATOR_SRCS := $(filter-out $(RAMULATOR_MAIN) $(RAMULATOR_SRCDIR)/Gem5Wrapper.cpp, $(wildcard $(RAMULATOR_SRCDIR)/*.cpp))
RAMULATOR_OBJS := $(patsubst $(RAMULATOR_SRCDIR)/%.cpp, $(RAMULATOR_OBJDIR)/%.o, $(RAMULATOR_SRCS))
RAMULATOR_INCDIR := $(RAMULATOR_SRCDIR)

# CXL Variables
CXL_ROOT := .
CXL_SRCDIR := $(CXL_ROOT)/src
CXL_OBJDIR := $(CXL_ROOT)/obj
CXL_INCDIR := $(CXL_ROOT)/include
CXL_SRCS := $(filter-out $(CXL_SRCDIR)/Main.cpp, $(wildcard $(CXL_SRCDIR)/*.cpp))
CXL_OBJS := $(patsubst $(CXL_SRCDIR)/%.cpp, $(CXL_OBJDIR)/%.o, $(CXL_SRCS))
CXL_MAIN := $(CXL_SRCDIR)/Main.cpp

# GCC Flags
CXX := g++
OPT := -O0
CXXFLAGS := $(OPT) -std=c++17 -g -Wall -DRAMULATOR
INCDIR := -I$(RAMULATOR_INCDIR) -I$(CXL_INCDIR)

# Set CXL FLAGS based on command line args
CXLFLAGS := 
COPTS :=

ifeq ($(EVENTLOG),true)
	CXLFLAGS += -DEVENTLOG
endif
ifeq ($(DUMP),true)
	CXLFLAGS += -DDUMP
endif
ifeq ($(SKIP_CYCLE),true)
	CXLFLAGS += -DSKIP_CYCLE
endif
ifeq ($(TRACK_LATENCY),true)
	CXLFLAGS += -DTRACK_LATENCY
endif

CXXFLAGS += $(COPTS)


# Make targets

all: ramulator.t cxlsim

ramulator.t: $(RAMULATOR_OBJDIR) $(RAMULATOR_OBJS)
	$(info Building Ramulator)
	@touch ramulator.txt

$(RAMULATOR_OBJDIR)/%.o: $(RAMULATOR_SRCDIR)/%.cpp
	$(info Building ramulator objs)
	$(CXX) $(CXXFLAGS) $(CXLFLAGS) -I$(RAMULATOR_INCDIR) -I$(CXL_INCDIR) -c $< -o $@

$(RAMULATOR_OBJDIR):
	@mkdir -p $(RAMULATOR_OBJDIR)

cxlsim: $(CXL_OBJDIR) $(CXL_OBJS) $(RAMULATOR_OBJS) $(CXL_MAIN)
	$(info Building CXLSIM)
	$(CXX) $(CXXFLAGS) $(CXLFLAGS) -I$(RAMULATOR_INCDIR) -I$(CXL_INCDIR) $(RAMULATOR_OBJS) $(CXL_OBJS) $(CXL_MAIN) -o cxlsim

$(CXL_OBJDIR):
	@mkdir -p $(CXL_OBJDIR)

$(CXL_OBJDIR)/%.o: $(CXL_SRCDIR)/%.cpp
	$(info Building CXLSIM objs)
	$(CXX) $(CXXFLAGS) $(CXLFLAGS) -I$(RAMULATOR_INCDIR) -I$(CXL_INCDIR) -c $< -o $@

clean: clean_gen
	$(info Cleaning)
	rm -f ramulator/obj/*
	rm -f obj/*

clean_gen:
	rm -f *.csv
	rm -f *.log
	rm -f *.out
	rm -f *.dump
