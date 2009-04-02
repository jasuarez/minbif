CMAKE_OPTIONS = -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=0

all: build.bitlbee/Makefile
	make -C build.bitlbee all

build.bitlbee/Makefile:
	[ -d build.bitlbee ] || mkdir build.bitlbee; \
	cd build.bitlbee && cmake $(CMAKE_OPTIONS) ..

install:
	make -C build.bitlbee install

clean:
	rm -rf build.bitlbee
	rm -rf release

doc:
	cd doc/ && /usr/bin/doxygen

.PHONY: all clean install doc
