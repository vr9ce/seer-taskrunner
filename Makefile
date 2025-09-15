SHELL = /bin/bash -O globstar

.PHONY: all clean

all: bin/taskrunner

clean:
	rm -rf build bin
	rm -f ./**/?*~ ./**/.?*~ ./**/\#?*\# ./**/.\#?*\#

bin/taskrunner: build/CMakeLists.txt | bin/
	cd build && make

build/CMakeLists.txt: | build/
	cd build && cmake --preset default -DCMAKE_BUILD_TYPE=Debug ..

%/:
	mkdir -p $@
