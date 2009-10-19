ENABLE_CACA ?= ON
ENABLE_VIDEO ?= OFF
CMAKE_OPTIONS = -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=0 -DENABLE_CACA=$(ENABLE_CACA) -DENABLE_VIDEO=$(ENABLE_VIDEO)

all: build.minbif/Makefile
	make -C build.minbif all

build.minbif/Makefile:
	[ -d build.minbif ] || mkdir build.minbif; \
	cd build.minbif && cmake $(CMAKE_OPTIONS) .. || cd .. && rm -rf build.minbif

install:
	make -C build.minbif install

clean:
	rm -rf build.minbif
	rm -rf release

doc:
	cd doc/ && /usr/bin/doxygen

tests:
	make -C tests

.PHONY: all clean install doc tests
