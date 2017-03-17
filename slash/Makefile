CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -std=c++11 -D__STDC_FORMAT_MACROS -std=c++11 -gdwarf-2 -Wno-redundant-decls
else
	CXXFLAGS = -O2 -g -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -gdwarf-2 -Wno-redundant-decls -Wno-sign-compare
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ -D__STDC_FORMAT_MACROS -fPIC -std=c++11 -gdwarf-2
endif

SRC_DIR = ./src/
LIB_DIR = ./lib
TEST_DIR = ./test/

INCLUDE_PATH = -I./
LIB_PATH = -L./ \


LIBS = -lpthread

LIBRARY = libslash.a
OUTPUT_LIB = $(LIB_DIR)/$(LIBRARY)

TESTS = \
  $(TEST_DIR)/slash_string_test \
  $(TEST_DIR)/slash_binlog_test \
  $(TEST_DIR)/base_conf_test

.PHONY: all clean check


BASE_OBJS := $(wildcard $(SRC_DIR)/*.cc)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.c)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_OBJS))


all: $(OUTPUT_LIB)
	make -C example 
	@echo "Success, go, go, go..."

$(OUTPUT_LIB): $(LIBRARY)
	mkdir -p $(LIB_DIR)
	mv $< $(LIB_DIR)

$(LIBRARY): $(OBJS)
	rm -rf $@
	ar -rcs $@ $(OBJS)

$(OBJECT): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDE_PATH) $(LIB_PATH) -Wl,-Bdynamic $(LIBS)

$(OBJS): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE_PATH) 

clean: 
	make -C example clean
	rm -rf $(SRC_DIR)/*.o
	rm -rf $(LIB_DIR)
	rm -rf $(TESTS_DIR)

check: $(OUTPUT_LIB) $(TESTS)
	for t in $(notdir $(TESTS)); do echo "***** Running $$t"; $(TESTS_DIR)/$$t || exit 1; done

TESTHARNESS = $(TESTS_DIR)/testharness.cc

$(TESTS_DIR)/slash_string_test: $(TESTS_DIR)/slash_string_test.cc $(TESTHARNESS) $(OUTPUT_LIB) $(LIBS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $^ -o $@

$(TESTS_DIR)/slash_binlog_test: $(TESTS_DIR)/slash_binlog_test.cc $(TESTHARNESS) $(OUTPUT_LIB) $(LIBS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $^ -o $@

$(TESTS_DIR)/base_conf_test: $(TESTS_DIR)/base_conf_test.cc $(TESTHARNESS) $(OUTPUT_LIB) $(LIBS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $^ -o $@
