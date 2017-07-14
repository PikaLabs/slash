include ../make_config.mk

.PHONY: clean libslash

all: conf_example cond_lock_example binlog_example mutex_example hash_example

conf_example: libslash conf_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../lib/libslash.a -I../.. -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

cond_lock_example: libslash cond_lock_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../lib/libslash.a -I../.. -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

binlog_example: libslash binlog_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../lib/libslash.a -I../.. -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

mutex_example: libslash mutex_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../lib/libslash.a -I../.. -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

hash_example: libslash hash_example.cc
	$(CXX) $(CXXFLAGS) $@.cc -o$@ ../lib/libslash.a -I../.. -O2 -std=c++11 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS)

clean:
	find . -name "*.[oda]" -exec rm -f {} \;
	rm -rf ./conf_example ./cond_lock_example ./binlog_example ./mutex_example ./hash_example

libslash:
	cd .. && $(MAKE) static_lib
