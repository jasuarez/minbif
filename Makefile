ENABLE_CACA ?= ON
CMAKE_OPTIONS = -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=0 -DENABLE_CACA=$(ENABLE_CACA)

all: build.minbif/Makefile
	make -C build.minbif all

build.minbif/Makefile:
	[ -d build.minbif ] || mkdir build.minbif; \
	cd build.minbif && cmake $(CMAKE_OPTIONS) ..

install:
	make -C build.minbif install

clean:
	rm -rf build.minbif
	rm -rf release

doc:
	cd doc/ && /usr/bin/doxygen

.PHONY: all clean install doc
