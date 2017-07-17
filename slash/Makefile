CLEAN_FILES = # deliberately empty, so we can append below.
CC=cc
CXX=g++
LDFLAGS= -lpthread -lrt $(EXTRA_LDFLAGS)
CFLAGS= -DOS_LINUX -fno-builtin-memcmp -msse -msse4.2 ${EXTRA_CFLAGS}
CXXFLAGS=-std=c++11 -DOS_LINUX -fno-builtin-memcmp -msse -msse4.2 ${EXTRA_CXXFLAGS}
SHARED_CFLAGS=-fPIC
SHARED_LDFLAGS=-Wl,--no-as-needed -shared -Wl,-soname -Wl,
PROFILING_FLAGS=-pg
ARFLAGS = rs
OPT=

# Set the default DEBUG_LEVEL to 0
DEBUG_LEVEL?=0

ifeq ($(MAKECMDGOALS),dbg)
  DEBUG_LEVEL=2 # compatible with rocksdb
endif

ifeq ($(MAKECMDGOALS),all)
  DEBUG_LEVEL=0
endif

ifeq ($(MAKECMDGOALS),clean)
  DEBUG_LEVEL=0
endif

ifeq ($(MAKECMDGOALS),shared_lib)
  DEBUG_LEVEL=0
endif

ifeq ($(MAKECMDGOALS),static_lib)
  DEBUG_LEVEL=0
endif

# compile with -O2 if for release
# if we're compiling for release, compile without debug code (-DNDEBUG) and
# don't treat warnings as errors
ifeq ($(DEBUG_LEVEL),0)
DISABLE_WARNING_AS_ERROR=1
OPT += -O2 -fno-omit-frame-pointer -DNDEBUG
# Skip for archs that don't support -momit-leaf-frame-pointer
ifeq (,$(shell $(CXX) -fsyntax-only -momit-leaf-frame-pointer -xc /dev/null 2>&1))
OPT += -momit-leaf-frame-pointer
endif
else
$(warning Warning: Compiling in debug mode. Don't use the resulting binary in production)
OPT += -O0 -D__XDEBUG__ -D_GNU_SOURCE $(PROFILING_FLAGS)
endif

#-----------------------------------------------

LIB_SOURCES = \
	src/build_version.cc \
	src/base_conf.cc \
	src/cond_lock.cc \
	src/env.cc \
	src/posix.cc \
	src/rsync.cc \
	src/slash_binlog_impl.cc \
	src/slash_coding.cc \
	src/slash_hash.cc \
	src/slash_mutex.cc \
	src/slash_status.cc \
	src/slash_string.cc \
	src/slash_testharness.cc \
	src/testutil.cc \

AM_DEFAULT_VERBOSITY = 0

AM_V_GEN = $(am__v_GEN_$(V))
am__v_GEN_ = $(am__v_GEN_$(AM_DEFAULT_VERBOSITY))
am__v_GEN_0 = @echo "  GEN     " $@;
am__v_GEN_1 =
AM_V_at = $(am__v_at_$(V))
am__v_at_ = $(am__v_at_$(AM_DEFAULT_VERBOSITY))
am__v_at_0 = @
am__v_at_1 =

AM_V_CC = $(am__v_CC_$(V))
am__v_CC_ = $(am__v_CC_$(AM_DEFAULT_VERBOSITY))
am__v_CC_0 = @echo "  CC      " $@;
am__v_CC_1 =
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
AM_V_CCLD = $(am__v_CCLD_$(V))
am__v_CCLD_ = $(am__v_CCLD_$(AM_DEFAULT_VERBOSITY))
am__v_CCLD_0 = @echo "  CCLD    " $@;
am__v_CCLD_1 =
AM_V_AR = $(am__v_AR_$(V))
am__v_AR_ = $(am__v_AR_$(AM_DEFAULT_VERBOSITY))
am__v_AR_0 = @echo "  AR      " $@;
am__v_AR_1 =

AM_LINK = $(AM_V_CCLD)$(CXX) $^ $(EXEC_LDFLAGS) -o $@ $(LDFLAGS) $(COVERAGEFLAGS)

CFLAGS += -g
CXXFLAGS += -g

# This (the first rule) must depend on "all".
default: all

WARNING_FLAGS = -W -Wextra -Wall -Wsign-compare \
  -Wno-unused-parameter -Wno-redundant-decls -Wwrite-strings \
	-Wpointer-arith -Wreorder -Wswitch -Wsign-promo

ifndef DISABLE_WARNING_AS_ERROR
  WARNING_FLAGS += -Werror
endif

CFLAGS += $(WARNING_FLAGS) -I.. $(OPT)
CXXFLAGS += $(WARNING_FLAGS) -I.. $(OPT) -Woverloaded-virtual -Wnon-virtual-dtor -Wno-missing-field-initializers

LDFLAGS += $(PLATFORM_LDFLAGS)

date := $(shell date +%F)
git_sha := $(shell git rev-parse HEAD 2>/dev/null)
gen_build_version = sed -e s/@@GIT_SHA@@/$(git_sha)/ -e s/@@GIT_DATE_TIME@@/$(date)/ src/build_version.cc.in
# Record the version of the source that we are compiling.
# We keep a record of the git revision in this file.  It is then built
# as a regular source file as part of the compilation process.
# One can run "strings executable_filename | grep _build_" to find
# the version of the source that we used to build the executable file.
CLEAN_FILES += src/build_version.cc

src/build_version.cc: FORCE
	$(AM_V_GEN)rm -f $@-t
	$(AM_V_at)$(gen_build_version) > $@-t
	$(AM_V_at)if test -f $@; then         \
	  cmp -s $@-t $@ && rm -f $@-t || mv -f $@-t $@;    \
	else mv -f $@-t $@; fi
FORCE: 

LIBOBJECTS = $(LIB_SOURCES:.cc=.o)

# if user didn't config LIBNAME, set the default
ifeq ($(LIBNAME),)
# we should only run slash in production with DEBUG_LEVEL 0
ifeq ($(DEBUG_LEVEL),0)
        LIBNAME=libslash
else
        LIBNAME=libslash_debug
endif
endif
LIBOUTPUT = ./lib
dummy := $(shell mkdir -p $(LIBOUTPUT))
LIBRARY = $(LIBOUTPUT)/${LIBNAME}.a

SLASH_MAJOR = $(shell egrep "SLASH_MAJOR.[0-9]" include/version.h | cut -d ' ' -f 3)
SLASH_MINOR = $(shell egrep "SLASH_MINOR.[0-9]" include/version.h | cut -d ' ' -f 3)
SLASH_PATCH = $(shell egrep "SLASH_PATCH.[0-9]" include/version.h | cut -d ' ' -f 3)

#-----------------------------------------------
# Create platform independent shared libraries.
#-----------------------------------------------
PLATFORM_SHARED_EXT=so

SHARED_MAJOR = $(SLASH_MAJOR)
SHARED_MINOR = $(SLASH_MINOR)
SHARED_PATCH = $(SLASH_PATCH)
SHARED1 = ${LIBNAME}.$(PLATFORM_SHARED_EXT)
SHARED2 = $(SHARED1).$(SHARED_MAJOR)
SHARED3 = $(SHARED1).$(SHARED_MAJOR).$(SHARED_MINOR)
SHARED4 = $(SHARED1).$(SHARED_MAJOR).$(SHARED_MINOR).$(SHARED_PATCH)
SHARED = $(SHARED1) $(SHARED2) $(SHARED3) $(SHARED4)
$(SHARED1): $(SHARED4)
	ln -fs $(SHARED4) $(SHARED1)
$(SHARED2): $(SHARED4)
	ln -fs $(SHARED4) $(SHARED2)
$(SHARED3): $(SHARED4)
	ln -fs $(SHARED4) $(SHARED3)

$(SHARED4): $(LIBOBJECTS)
	$(CXX) $(SHARED_LDFLAGS)$(SHARED3) $(CXXFLAGS) $(SHARED_CFLAGS) $(LIB_SOURCES) \
    $(LDFLAGS) -o $@

TESTS = slash_string_test slash_binlog_test slash_coding_test base_conf_test \
				slash_env_test

EXAMPLES = conf_example cond_lock_example binlog_example mutex_example hash_example

.PHONY: clean dbg static_lib shared_lib all check example

all: $(LIBRARY)

static_lib: $(LIBRARY)

shared_lib: $(SHARED)
	for f in $(notdir $(SHARED)); do mv "$$f" $(LIBOUTPUT); done

example: $(EXAMPLES)

check: $(LIBRARY) $(TESTS)
	for t in $(notdir $(TESTS)); do echo "***** Running $$t"; ./$$t || exit 1; done

dbg: $(LIBRARY) $(EXAMPLES)

$(LIBRARY): $(LIBOBJECTS)
	$(AM_V_AR)rm -f $@
	$(AM_V_at)$(AR) $(ARFLAGS) $@ $(LIBOBJECTS)

clean:
	make -C ./examples clean
	rm -f $(TESTS) $(EXAMPLES)
	rm -f $(LIBRARY) $(SHARED)
	rm -rf $(CLEAN_FILES)
	rm -rf $(LIBOUTPUT)
	find . -name "*.[oda]" -exec rm -f {} \;
	find . -type f -regex ".*\.\(\(gcda\)\|\(gcno\)\)" -exec rm {} \;

.cc.o:
	$(AM_V_CC)$(CXX) $(CXXFLAGS) -c $< -o $@ $(COVERAGEFLAGS)

.c.o:
	$(AM_V_CC)$(CC) $(CFLAGS) -c $< -o $@

# tests

TEST_MAIN = tests/test_main.o

slash_string_test: tests/slash_string_test.o $(TEST_MAIN) $(LIBOBJECTS)
	$(AM_LINK)

slash_binlog_test: tests/slash_binlog_test.o $(TEST_MAIN) $(LIBOBJECTS)
	$(AM_LINK)

base_conf_test: tests/base_conf_test.o $(TEST_MAIN) $(LIBOBJECTS)
	$(AM_LINK)

slash_coding_test: tests/slash_coding_test.o $(TEST_MAIN) $(LIBOBJECTS)
	$(AM_LINK)

slash_env_test: tests/slash_env_test.o $(TEST_MAIN) $(LIBOBJECTS)
	$(AM_LINK)

# examples

conf_example: examples/conf_example.o $(LIBOBJECTS)
	$(AM_LINK)

cond_lock_example: examples/cond_lock_example.o $(LIBOBJECTS)
	$(AM_LINK)

binlog_example: examples/binlog_example.o $(LIBOBJECTS)
	$(AM_LINK)

mutex_example: examples/mutex_example.o $(LIBOBJECTS)
	$(AM_LINK)

hash_example: examples/hash_example.o $(LIBOBJECTS)
	$(AM_LINK)
