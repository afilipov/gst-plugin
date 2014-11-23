# Define programs and commands.
SH     = sh
# Archive-maintaining program;
AR     = ar
# Program for doing assembly;
AS     = as
# Program for compiling C programs;
CC     = ${CROSS_COMPILE}gcc
# Program for compiling C++ programs;
CXX    = ${CROSS_COMPILE}g++
# Program for running the C preprocessor, with results to standard output;
CPP    = $(CC) -E
# Command to remove a file;
RM     = rm -f
# Command make a file;
MAKE   = make

# Target executable file name
TARGET =

# Target library file name
OUTLIB  = gstgvision

# Project
PRJDIR  = .
PRJINC  = $(PRJDIR)/include
PRJSRC  = $(PRJDIR)/src
PRJOBJ  = $(PRJDIR)/obj
PRJLIB  = $(PRJDIR)/lib
PRJBIN  = $(PRJDIR)/bin

# Additional project with separate makefile
PRJSUB =

EXTINC =  $(PRJINC) /usr/include/gstreamer-1.0 /usr/include/glib-2.0 /usr/lib/x86_64-linux-gnu/glib-2.0/include

# Additional libraries "-lcommon"
EXTLIB	= -lgstbase-1.0 -lgstcontroller-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lpthread

# Place -I options here
INCLUDES = -I. $(addprefix -I,$(EXTINC))

# List C and CPP source files here. (C dependencies are automatically generated.)
SOURCES = \
	gvision.c \
	gvision_base.c \
	gvision_multithread.c \
	defisheye/gvision_defisheye.c \
	histogram/gvision_histogram.c \
	convert/gvision_convert.c \
	hashmap/gvision_hash.c \
	hashmap/cutils/hashmap.c \
	duration/gvision_duration.c \
	gnuplot/gvision_gnuplot.c

# Add source directory prefix
ALLSOURCES = $(addprefix $(PRJSRC)/, $(SOURCES))

# Prepare "C" files list
CFILES   = $(filter %.c,   $(ALLSOURCES))
# Prepare "C++" files list
CPPFILES = $(filter %.cpp, $(ALLSOURCES))

# Define "C" object files.
COBJS	= $(patsubst $(PRJSRC)/%,$(PRJOBJ)/%,$(CFILES:%.c=%.o))
# Define "C++" object files.
CPPOBJS	= $(patsubst $(PRJSRC)/%,$(PRJOBJ)/%,$(CPPFILES:%.cpp=%.o))
# Define all object files.
OBJS	= $(COBJS) $(CPPOBJS)
# Define all dependencies
DEPS    = $(OBJS:%.o=%.d)

# Define output executable target
OUTBIN = $(PRJBIN)/$(TARGET)

# Define dynamic library file name
OUTDLIB = $(PRJLIB)/lib$(OUTLIB).so

# Define static library file name
OUTSLIB = $(PRJLIB)/lib$(OUTLIB).a

# Place -D or -U options here
DEF = -DHAVE_CONFIG_H -D_GNU_SOURCE -DMULTI_THREAD -DCALC_TOTAL_DURATION -DCALC_PDF_DURATION -DCALC_THREAD_DURATION

# Define CPU flags "-march=cpu-type"
CPU =

# Define specific MPU flags "-mno-fp-ret-in-387"
MPU =

# Define "C" standart
STD = -std=gnu99

# Global flags
OPT  = $(MPU) $(MPU) $(STD) -fPIC -O3

# Generate dependencies
DEP  = -MMD

WRN  = -Wall -Wclobbered -Wempty-body -Wignored-qualifiers -Wmissing-field-initializers -Wsign-compare -Wtype-limits -Wuninitialized

# Define only valid "C" optimization flags
COPT = $(OPT) -fomit-frame-pointer -fno-stack-check
# Define only valid "C" warnings flags
CWRN = $(WRN) \
	-Wmissing-parameter-type -Wold-style-declaration -Wimplicit-int -Wimplicit-function-declaration -Wimplicit -Wignored-qualifiers \
	-Wformat-nonliteral -Wcast-align -Wpointer-arith -Wbad-function-cast -Wmissing-prototypes -Wstrict-prototypes -Wmissing-declarations \
	-Wnested-externs -Wshadow -Wwrite-strings -Wfloat-equal -Woverride-init

# Define only valid "C++" optimization flags
CXXOPT  = $(COPT) -frtti
# Define only valid "C" warnings flags
CXXWRN  = $(WRN) \
	-Woverloaded-virtual \
	-Wignored-qualifiers \
	-Wformat-nonliteral -Wcast-align -Wpointer-arith -Wmissing-declarations \
	-Wcast-qual -Wwrite-strings -Wshadow -Wfloat-equal

# Extra flags to give to the C compiler.
CFLAGS = $(MCU) $(DEF) $(DEP) $(COPT) $(CWRN)

# Extra flags to give to the C++ compiler.
CXXFLAGS = $(MCU) $(DEF) $(DEP) $(CXXOPT) $(CXXWRN)

# Extra flags to give to the C compiler.
override CFLAGS += $(INCLUDES)

# Extra flags to give to the C++ compiler.
override CXXFLAGS += $(INCLUDES)

# Define global linker flags
EXFLAGS = $(EXTLIB) -L $(PRJLIB)

# Linker flags to give to the linker.
override LDFLAGS = $(EXFLAGS) -Wl,--no-as-needed -Wl,-ldl

# Special linker flags to build shared library
override SHFLAGS = $(EXFLAGS) -shared -s

# Executable not defined
ifeq ($(TARGET),)
 # Output library defined
 ifneq ($(OUTLIB),)
  all: $(OUTDLIB) $(OUTSLIB)
 else
  all:
	@echo "WARNING: Neither output, neither library defined!"
 endif
# Executable defined
else
 # Executable defined, but output library not defined
 ifeq ($(OUTLIB),)
  all: $(OUTBIN)
 # Executable and output library defined
 else
  all: $(OUTDLIB) $(OUTSLIB) $(OUTBIN)
 endif
endif
  # Sub-project defined
  ifneq ($(PRJSUB),)
	$(MAKE) -C $(PRJSUB) all
  endif

# Executable defined
ifneq ($(TARGET),)
 # Output library not defined
 ifeq ($(OUTLIB),)
  $(OUTBIN): $(OBJS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) src/$(TARGET).c -o $(OUTBIN) $(LDFLAGS) $(OBJS)
 # Executable and output library defined
 else
  $(OUTBIN): $(OUTSLIB) $(OUTDLIB)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) src/$(TARGET).c -o $(OUTBIN) $(LDFLAGS) $(OUTDLIB)
 endif
endif

ifneq ($(OUTLIB),)
 $(OUTDLIB): $(OBJS)
	mkdir -p $(@D)
	$(CXX) -o $@ $^ $(SHFLAGS)

 $(OUTSLIB): $(OBJS)
	mkdir -p $(@D)
	$(AR) rcs -o $@ $^
endif

$(COBJS): $(PRJOBJ)/%.o: $(PRJSRC)/%.c
	mkdir -p $(@D)
	$(CC) -c -o $@ $< $(DEP) $(CFLAGS)

$(CPPOBJS): $(PRJOBJ)/%.o: $(PRJSRC)/%.cpp
	mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(DEP) $(CXXFLAGS)

$(PRJBIN):
	mkdir -p $@

$(PRJOBJ):
	mkdir -p $@

$(PRJLIB):
	mkdir -p $@

# Target: clean project.
clean:
ifneq ($(PRJSUB),)
	$(MAKE) -C $(PRJSUB) clean
endif
	@echo Cleaning objects
	$(RM) -rf $(PRJOBJ)
	$(RM) $(PRJBIN)/$(TARGET).d

# Target: clean all.
distclean:
ifneq ($(PRJSUB),)
	$(MAKE) -C $(PRJSUB) distclean
endif
	@echo Cleaning all objects and executable
	$(RM) $(OBJS)
	$(RM) $(DEPS)

	$(RM) -rf $(PRJLIB)
	$(RM) -rf $(PRJBIN)
	$(RM) -rf $(PRJOBJ)

# Listing of phony targets.
.PHONY : all clean $(OUTDLIB) $(OUTSLIB) $(OUTBIN)

-include subsys_config.mk
-include $(DEPS)
