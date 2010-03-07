include Makefile.options.defaults
-include Makefile.options.local

include Makefile.parser

CMAKE_OPTIONS = $(EXTRA_CMAKE_FLAGS) $(CMAKE_PREFIX)

all: build/Makefile
	$(MAKE) -C build all

build/Makefile:
	@[ -d build ] || mkdir build
	cd build && cmake .. $(CMAKE_OPTIONS) || cd .. && rm -rf build

install:
	$(MAKE) -C build install

clean:
	rm -rf build
	rm -rf release

doc:
	cd doc/ && /usr/bin/doxygen

tests:
	$(MAKE) -C tests

.PHONY: all clean install doc tests
