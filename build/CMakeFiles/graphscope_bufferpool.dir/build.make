# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

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
CMAKE_SOURCE_DIR = /data/zhengyang/data/GraphScope_bufferpool

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /data/zhengyang/data/GraphScope_bufferpool/build

# Include any dependencies generated for this target.
include CMakeFiles/graphscope_bufferpool.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/graphscope_bufferpool.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/graphscope_bufferpool.dir/flags.make

CMakeFiles/graphscope_bufferpool.dir/main.cpp.o: CMakeFiles/graphscope_bufferpool.dir/flags.make
CMakeFiles/graphscope_bufferpool.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/graphscope_bufferpool.dir/main.cpp.o"
	/usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/graphscope_bufferpool.dir/main.cpp.o -c /data/zhengyang/data/GraphScope_bufferpool/main.cpp

CMakeFiles/graphscope_bufferpool.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/graphscope_bufferpool.dir/main.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /data/zhengyang/data/GraphScope_bufferpool/main.cpp > CMakeFiles/graphscope_bufferpool.dir/main.cpp.i

CMakeFiles/graphscope_bufferpool.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/graphscope_bufferpool.dir/main.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /data/zhengyang/data/GraphScope_bufferpool/main.cpp -o CMakeFiles/graphscope_bufferpool.dir/main.cpp.s

# Object files for target graphscope_bufferpool
graphscope_bufferpool_OBJECTS = \
"CMakeFiles/graphscope_bufferpool.dir/main.cpp.o"

# External object files for target graphscope_bufferpool
graphscope_bufferpool_EXTERNAL_OBJECTS =

../bin/graphscope_bufferpool: CMakeFiles/graphscope_bufferpool.dir/main.cpp.o
../bin/graphscope_bufferpool: CMakeFiles/graphscope_bufferpool.dir/build.make
../bin/graphscope_bufferpool: ../lib/libbufferpool.a
../bin/graphscope_bufferpool: CMakeFiles/graphscope_bufferpool.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/graphscope_bufferpool"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/graphscope_bufferpool.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/graphscope_bufferpool.dir/build: ../bin/graphscope_bufferpool

.PHONY : CMakeFiles/graphscope_bufferpool.dir/build

CMakeFiles/graphscope_bufferpool.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/graphscope_bufferpool.dir/cmake_clean.cmake
.PHONY : CMakeFiles/graphscope_bufferpool.dir/clean

CMakeFiles/graphscope_bufferpool.dir/depend:
	cd /data/zhengyang/data/GraphScope_bufferpool/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /data/zhengyang/data/GraphScope_bufferpool /data/zhengyang/data/GraphScope_bufferpool /data/zhengyang/data/GraphScope_bufferpool/build /data/zhengyang/data/GraphScope_bufferpool/build /data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles/graphscope_bufferpool.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/graphscope_bufferpool.dir/depend
