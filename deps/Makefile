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
OUTPUT = $(CURDIR)/output
THIRD_PATH = $(CURDIR)

# ----------------Dependences-------------------

ifndef SLASH_PATH
SLASH_PATH = $(THIRD_PATH)/slash
endif
SLASH = $(SLASH_PATH)/slash/lib/libslash$(DEBUG_SUFFIX).a

ifndef PINK_PATH
PINK_PATH = $(THIRD_PATH)/pink
endif
PINK = $(PINK_PATH)/pink/lib/libpink$(DEBUG_SUFFIX).a

ifndef JEMALLOC_PATH
JEMALLOC_PATH = $(THIRD_PATH)/jemalloc
endif

ifndef ZLIB_PATH
ZLIB_PATH = $(THIRD_PATH)/zlib
endif

ifndef SNAPPY_PATH
SNAPPY_PATH = $(THIRD_PATH)/snappy
endif

ifndef ROCKSDB_PATH
ROCKSDB_PATH = $(THIRD_PATH)/rocksdb
endif

ifndef BZIP2_PATH
BZIP2_PATH = $(THIRD_PATH)/bzip2
endif


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

default: all
.PHONY: default

slash:
	$(AM_V_at)make -C $(SLASH_PATH)/slash/ DEBUG_LEVEL=$(DEBUG_LEVEL)
.PHONY: slash

pink:
	$(AM_V_at)make -C $(PINK_PATH)/pink/ DEBUG_LEVEL=$(DEBUG_LEVEL) NO_PB=1 SLASH_PATH=$(SLASH_PATH)
.PHONY: pink

jemalloc:
	cd $(JEMALLOC_PATH) && \
	    sh autogen.sh && \
	    sh configure && \
	    make -j8
.PHONY: jemalloc

snappy:
	cd $(SNAPPY_PATH) && \
	cmake . &&\
	make -j8 snappy
.PHONY: snappy

zlib:
	cd $(ZLIB_PATH) && \
	    sh configure && \
	    make -j8
.PHONY: zlib


bzip2:
	cd $(BZIP2_PATH) && \
	    cmake . && \
	    make -j8
.PHONY: bzip2

rocksdb:
	cd $(ROCKSDB_PATH) && \
	make -j10 OPT="-DSNAPPY -I$(SNAPPY_PATH) -L$(SNAPPY_PATH) -DJEMALLOC -I$(JEMALLOC_PATH)/include -L$(JEMALLOC_PATH)/lib -L$(ZLIB_PATH) --rtti" static_lib
.PHONY: rocksdb

all: jemalloc slash pink snappy zlib bzip2 rocksdb
.PHONY: all

clean:
	make -C $(PINK_PATH)/pink/ SLASH_PATH=$(SLASH_PATH) clean
	make -C $(SLASH_PATH)/slash/ clean
	make -C $(JEMALLOC_PATH)/ clean
	make -C $(ROCKSDB_PATH)/ clean
	make -C $(ZLIB_PATH)/ distclean
	make -C $(SNAPPY_PATH)/ clean
	make -C $(BZIP2_PATH)/ clean
.PHONY: clean

distclean: clean
	make -C $(PINK_PATH)/pink/ SLASH_PATH=$(SLASH_PATH) distclean
	make -C $(SLASH_PATH)/slash/ distclean
	make -C $(JEMALLOC_PATH)/ distclean
	make -C $(ROCKSDB_PATH)/ distclean
	make -C $(ZLIB_PATH)/ distclean
	make -C $(SNAPPY_PATH)/ distclean
	make -C $(BZIP2_PATH)/ distclean
.PHONY: distclean
