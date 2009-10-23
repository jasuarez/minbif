ENABLE_MINBIF ?= ON
ENABLE_CACA ?= ON
ENABLE_VIDEO ?= OFF
ENABLE_PLUGIN ?= OFF
CMAKE_OPTIONS = -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=0 \
                -DENABLE_CACA=$(ENABLE_CACA) -DENABLE_VIDEO=$(ENABLE_VIDEO) -DENABLE_PLUGIN=$(ENABLE_PLUGIN) -DENABLE_MINBIF=$(ENABLE_MINBIF) \
                -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DMAN_PREFIX=$(MAN_PREFIX) -DCONF_PREFIX=$(CONF_PREFIX) -DDOC_PREFIX=$(DOC_PREFIX)

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
