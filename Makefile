CLEAN_FILES = # deliberately empty, so we can append below.
CXX=g++
PLATFORM_LDFLAGS= -lpthread -lrt
PLATFORM_CXXFLAGS= -std=c++11 -fno-builtin-memcmp -msse -msse4.2
PROFILING_FLAGS=-pg
OPT=
LDFLAGS += -Wl,-rpath=$(RPATH)

# DEBUG_LEVEL can have two values:
# * DEBUG_LEVEL=2; this is the ultimate debug mode. It will compile pika
# without any optimizations. To compile with level 2, issue `make dbg`
# * DEBUG_LEVEL=0; this is the debug level we use for release. If you're
# running pika in production you most definitely want to compile pika
# with debug level 0. To compile with level 0, run `make`,

# Set the default DEBUG_LEVEL to 0
DEBUG_LEVEL?=0


# compile with -O2 if debug level is not 2
ifneq ($(DEBUG_LEVEL), 2)
OPT += -O2 -fno-omit-frame-pointer
# if we're compiling for release, compile without debug code (-DNDEBUG) and
# don't treat warnings as errors
OPT += -DNDEBUG
DISABLE_WARNING_AS_ERROR=1
# Skip for archs that don't support -momit-leaf-frame-pointer
ifeq (,$(shell $(CXX) -fsyntax-only -momit-leaf-frame-pointer -xc /dev/null 2>&1))
OPT += -momit-leaf-frame-pointer
endif
else
$(warning Warning: Compiling in debug mode. Don't use the resulting binary in production)
OPT += $(PROFILING_FLAGS)
DEBUG_SUFFIX = "_debug"
endif



# ----------------------------------------------
OUTPUT = $(CURDIR)/build
THIRD_PATH = $(CURDIR)/deps
SRC_PATH = $(CURDIR)/src


#-----------------------------------------------

AM_DEFAULT_VERBOSITY = 0

AM_V_GEN = $(am__v_GEN_$(V))
am__v_GEN_ = $(am__v_GEN_$(AM_DEFAULT_VERBOSITY))
am__v_GEN_0 = @echo "  GEN     " $(notdir $@);
am__v_GEN_1 =
AM_V_at = $(am__v_at_$(V))
am__v_at_ = $(am__v_at_$(AM_DEFAULT_VERBOSITY))
am__v_at_0 = @
am__v_at_1 =

AM_V_CC = $(am__v_CC_$(V))
am__v_CC_ = $(am__v_CC_$(AM_DEFAULT_VERBOSITY))
am__v_CC_0 = @echo "  CC      " $(notdir $@);
am__v_CC_1 =
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
AM_V_CCLD = $(am__v_CCLD_$(V))
am__v_CCLD_ = $(am__v_CCLD_$(AM_DEFAULT_VERBOSITY))
am__v_CCLD_0 = @echo "  CCLD    " $(notdir $@);
am__v_CCLD_1 =

AM_LINK = $(AM_V_CCLD)$(CXX) $^ $(EXEC_LDFLAGS) -o $@ $(LDFLAGS)

CXXFLAGS += -g

all: deps vcdb
.PHONY: all

deps:
	cd $(THIRD_PATH) && \
	    make
.PHONY: deps

vcdb:
	 mkdir -p build && \
 	 cd build && \
 	 cmake .. && \
 	 make -j8

.PHONY: vcdb

clean:
	make -C $(THIRD_PATH) clean
.PHONY: clean

distclean:
	make -C $(THIRD_PATH) distclean
	rm -rf $(OUTPUT)
.PHONY: distclean

test:
	export PATH=$(PATH):$(PWD)/build && \
	./runtest.sh
.PHONY: test
