SHELL = /bin/bash -O globstar

.PHONY: all
all: bin/taskrunner

.PHONY: clean
clean:
	rm -rf build bin
	rm -f ./**/?*~ ./**/.?*~ ./**/\#?*\# ./**/.\#?*\#

.PHONY: test
test: bin/taskrunner | examples/
	@for task in examples/*.json; do  \
		echo -e \\n--------------------- "测试: $$task"  ---------------------;  \
		bin/taskrunner "$$task";  \
		if (($$?)); then  \
			echo $$'\n\e[31mFAILED!\e[0m';  \
		else  \
			echo $$'\n\e[32mSUCCEED\e[0m';  \
		fi;  \
	done

bin/taskrunner: FORCE build/Makefile | bin/
	cd build && make taskrunner

build/Makefile: CMakeLists.txt | build/
	cd build && cmake --preset default -DCMAKE_BUILD_TYPE=Debug ..

%/:
	mkdir -p $@

FORCE:
