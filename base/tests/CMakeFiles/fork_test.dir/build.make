# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/wanjun/Desktop/muduo-master

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/wanjun/Desktop/muduo-master/build

# Include any dependencies generated for this target.
include muduo/base/tests/CMakeFiles/fork_test.dir/depend.make

# Include the progress variables for this target.
include muduo/base/tests/CMakeFiles/fork_test.dir/progress.make

# Include the compile flags for this target's objects.
include muduo/base/tests/CMakeFiles/fork_test.dir/flags.make

muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o: muduo/base/tests/CMakeFiles/fork_test.dir/flags.make
muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o: ../muduo/base/tests/Fork_test.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wanjun/Desktop/muduo-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o"
	cd /home/wanjun/Desktop/muduo-master/build/muduo/base/tests && /usr/bin/clang++-6.0  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fork_test.dir/Fork_test.cc.o -c /home/wanjun/Desktop/muduo-master/muduo/base/tests/Fork_test.cc

muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fork_test.dir/Fork_test.cc.i"
	cd /home/wanjun/Desktop/muduo-master/build/muduo/base/tests && /usr/bin/clang++-6.0 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/wanjun/Desktop/muduo-master/muduo/base/tests/Fork_test.cc > CMakeFiles/fork_test.dir/Fork_test.cc.i

muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fork_test.dir/Fork_test.cc.s"
	cd /home/wanjun/Desktop/muduo-master/build/muduo/base/tests && /usr/bin/clang++-6.0 $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/wanjun/Desktop/muduo-master/muduo/base/tests/Fork_test.cc -o CMakeFiles/fork_test.dir/Fork_test.cc.s

muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o.requires:

.PHONY : muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o.requires

muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o.provides: muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o.requires
	$(MAKE) -f muduo/base/tests/CMakeFiles/fork_test.dir/build.make muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o.provides.build
.PHONY : muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o.provides

muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o.provides.build: muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o


# Object files for target fork_test
fork_test_OBJECTS = \
"CMakeFiles/fork_test.dir/Fork_test.cc.o"

# External object files for target fork_test
fork_test_EXTERNAL_OBJECTS =

bin/fork_test: muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o
bin/fork_test: muduo/base/tests/CMakeFiles/fork_test.dir/build.make
bin/fork_test: lib/libmuduo_base.a
bin/fork_test: muduo/base/tests/CMakeFiles/fork_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/wanjun/Desktop/muduo-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../../bin/fork_test"
	cd /home/wanjun/Desktop/muduo-master/build/muduo/base/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fork_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
muduo/base/tests/CMakeFiles/fork_test.dir/build: bin/fork_test

.PHONY : muduo/base/tests/CMakeFiles/fork_test.dir/build

muduo/base/tests/CMakeFiles/fork_test.dir/requires: muduo/base/tests/CMakeFiles/fork_test.dir/Fork_test.cc.o.requires

.PHONY : muduo/base/tests/CMakeFiles/fork_test.dir/requires

muduo/base/tests/CMakeFiles/fork_test.dir/clean:
	cd /home/wanjun/Desktop/muduo-master/build/muduo/base/tests && $(CMAKE_COMMAND) -P CMakeFiles/fork_test.dir/cmake_clean.cmake
.PHONY : muduo/base/tests/CMakeFiles/fork_test.dir/clean

muduo/base/tests/CMakeFiles/fork_test.dir/depend:
	cd /home/wanjun/Desktop/muduo-master/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/wanjun/Desktop/muduo-master /home/wanjun/Desktop/muduo-master/muduo/base/tests /home/wanjun/Desktop/muduo-master/build /home/wanjun/Desktop/muduo-master/build/muduo/base/tests /home/wanjun/Desktop/muduo-master/build/muduo/base/tests/CMakeFiles/fork_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : muduo/base/tests/CMakeFiles/fork_test.dir/depend

