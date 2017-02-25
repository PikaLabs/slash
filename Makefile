CXX = g++
ifeq ($(__PERF), 1)
	CXXFLAGS = -O0 -g -pg -pipe -fPIC -D__XDEBUG__ -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -std=c++11 -D__STDC_FORMAT_MACROS -std=c++11 -gdwarf-2 -Wno-redundant-decls
else
	CXXFLAGS = -O2 -g -pipe -fPIC -W -Wwrite-strings -Wpointer-arith -Wreorder -Wswitch -Wsign-promo -Wredundant-decls -Wformat -Wall -D_GNU_SOURCE -D__STDC_FORMAT_MACROS -std=c++11 -gdwarf-2 -Wno-redundant-decls -Wno-sign-compare
	# CXXFLAGS = -Wall -W -DDEBUG -g -O0 -D__XDEBUG__ -D__STDC_FORMAT_MACROS -fPIC -std=c++11 -gdwarf-2
endif

SRC_DIR = ./src/
OUTPUT = ./output/
TESTS_DIR = $(SRC_DIR)/tests/

INCLUDE_PATH = -I./ \
							 -I./include/ \
							 -I$(SRC_DIR) \
							 -I$(TESTS_DIR)

LIB_PATH = -L./ \


LIBS = -lpthread

LIBRARY = libslash.a
OUTPUT_LIB = $(OUTPUT)/lib/$(LIBRARY)

TESTS = \
  $(TESTS_DIR)/slash_string_test \
  $(TESTS_DIR)/slash_binlog_test \
  $(TESTS_DIR)/base_conf_test

.PHONY: all clean check


BASE_OBJS := $(wildcard $(SRC_DIR)/*.cc)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.c)
BASE_OBJS += $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cc,%.o,$(BASE_OBJS))


all: $(OUTPUT_LIB)
	@echo "Success, go, go, go..."

$(OUTPUT_LIB): $(LIBRARY)
	rm -rf $(OUTPUT)
	mkdir -p $(OUTPUT)/include
	mkdir -p $(OUTPUT)/lib
	cp -r ./include $(OUTPUT)/
	mv $< $(OUTPUT)/lib/

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
	rm -rf $(OUTPUT)/*
	rm -rf $(OUTPUT)
	rm -f $(TESTS_DIR)/*.o
	rm -f $(TESTS)

check: $(OUTPUT_LIB) $(TESTS)
	for t in $(notdir $(TESTS)); do echo "***** Running $$t"; $(TESTS_DIR)/$$t || exit 1; done

TESTHARNESS = $(TESTS_DIR)/testharness.cc

$(TESTS_DIR)/slash_string_test: $(TESTS_DIR)/slash_string_test.cc $(TESTHARNESS) $(OUTPUT_LIB) $(LIBS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $^ -o $@

$(TESTS_DIR)/slash_binlog_test: $(TESTS_DIR)/slash_binlog_test.cc $(TESTHARNESS) $(OUTPUT_LIB) $(LIBS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $^ -o $@

$(TESTS_DIR)/base_conf_test: $(TESTS_DIR)/base_conf_test.cc $(TESTHARNESS) $(OUTPUT_LIB) $(LIBS)
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATH) $^ -o $@
