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
include src/CMakeFiles/bufferpool.dir/depend.make

# Include the progress variables for this target.
include src/CMakeFiles/bufferpool.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/bufferpool.dir/flags.make

src/CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.o: src/CMakeFiles/bufferpool.dir/flags.make
src/CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.o: ../src/buffer_pool_manager.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.o"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.o -c /data/zhengyang/data/GraphScope_bufferpool/src/buffer_pool_manager.cpp

src/CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.i"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /data/zhengyang/data/GraphScope_bufferpool/src/buffer_pool_manager.cpp > CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.i

src/CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.s"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /data/zhengyang/data/GraphScope_bufferpool/src/buffer_pool_manager.cpp -o CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.s

src/CMakeFiles/bufferpool.dir/config.cpp.o: src/CMakeFiles/bufferpool.dir/flags.make
src/CMakeFiles/bufferpool.dir/config.cpp.o: ../src/config.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/CMakeFiles/bufferpool.dir/config.cpp.o"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/bufferpool.dir/config.cpp.o -c /data/zhengyang/data/GraphScope_bufferpool/src/config.cpp

src/CMakeFiles/bufferpool.dir/config.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/bufferpool.dir/config.cpp.i"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /data/zhengyang/data/GraphScope_bufferpool/src/config.cpp > CMakeFiles/bufferpool.dir/config.cpp.i

src/CMakeFiles/bufferpool.dir/config.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/bufferpool.dir/config.cpp.s"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /data/zhengyang/data/GraphScope_bufferpool/src/config.cpp -o CMakeFiles/bufferpool.dir/config.cpp.s

src/CMakeFiles/bufferpool.dir/disk_manager.cpp.o: src/CMakeFiles/bufferpool.dir/flags.make
src/CMakeFiles/bufferpool.dir/disk_manager.cpp.o: ../src/disk_manager.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/CMakeFiles/bufferpool.dir/disk_manager.cpp.o"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/bufferpool.dir/disk_manager.cpp.o -c /data/zhengyang/data/GraphScope_bufferpool/src/disk_manager.cpp

src/CMakeFiles/bufferpool.dir/disk_manager.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/bufferpool.dir/disk_manager.cpp.i"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /data/zhengyang/data/GraphScope_bufferpool/src/disk_manager.cpp > CMakeFiles/bufferpool.dir/disk_manager.cpp.i

src/CMakeFiles/bufferpool.dir/disk_manager.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/bufferpool.dir/disk_manager.cpp.s"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /data/zhengyang/data/GraphScope_bufferpool/src/disk_manager.cpp -o CMakeFiles/bufferpool.dir/disk_manager.cpp.s

src/CMakeFiles/bufferpool.dir/extendible_hash.cpp.o: src/CMakeFiles/bufferpool.dir/flags.make
src/CMakeFiles/bufferpool.dir/extendible_hash.cpp.o: ../src/extendible_hash.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/CMakeFiles/bufferpool.dir/extendible_hash.cpp.o"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/bufferpool.dir/extendible_hash.cpp.o -c /data/zhengyang/data/GraphScope_bufferpool/src/extendible_hash.cpp

src/CMakeFiles/bufferpool.dir/extendible_hash.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/bufferpool.dir/extendible_hash.cpp.i"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /data/zhengyang/data/GraphScope_bufferpool/src/extendible_hash.cpp > CMakeFiles/bufferpool.dir/extendible_hash.cpp.i

src/CMakeFiles/bufferpool.dir/extendible_hash.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/bufferpool.dir/extendible_hash.cpp.s"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /data/zhengyang/data/GraphScope_bufferpool/src/extendible_hash.cpp -o CMakeFiles/bufferpool.dir/extendible_hash.cpp.s

src/CMakeFiles/bufferpool.dir/lru_replacer.cpp.o: src/CMakeFiles/bufferpool.dir/flags.make
src/CMakeFiles/bufferpool.dir/lru_replacer.cpp.o: ../src/lru_replacer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object src/CMakeFiles/bufferpool.dir/lru_replacer.cpp.o"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/bufferpool.dir/lru_replacer.cpp.o -c /data/zhengyang/data/GraphScope_bufferpool/src/lru_replacer.cpp

src/CMakeFiles/bufferpool.dir/lru_replacer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/bufferpool.dir/lru_replacer.cpp.i"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /data/zhengyang/data/GraphScope_bufferpool/src/lru_replacer.cpp > CMakeFiles/bufferpool.dir/lru_replacer.cpp.i

src/CMakeFiles/bufferpool.dir/lru_replacer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/bufferpool.dir/lru_replacer.cpp.s"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /data/zhengyang/data/GraphScope_bufferpool/src/lru_replacer.cpp -o CMakeFiles/bufferpool.dir/lru_replacer.cpp.s

src/CMakeFiles/bufferpool.dir/page.cpp.o: src/CMakeFiles/bufferpool.dir/flags.make
src/CMakeFiles/bufferpool.dir/page.cpp.o: ../src/page.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object src/CMakeFiles/bufferpool.dir/page.cpp.o"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/bufferpool.dir/page.cpp.o -c /data/zhengyang/data/GraphScope_bufferpool/src/page.cpp

src/CMakeFiles/bufferpool.dir/page.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/bufferpool.dir/page.cpp.i"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /data/zhengyang/data/GraphScope_bufferpool/src/page.cpp > CMakeFiles/bufferpool.dir/page.cpp.i

src/CMakeFiles/bufferpool.dir/page.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/bufferpool.dir/page.cpp.s"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /data/zhengyang/data/GraphScope_bufferpool/src/page.cpp -o CMakeFiles/bufferpool.dir/page.cpp.s

# Object files for target bufferpool
bufferpool_OBJECTS = \
"CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.o" \
"CMakeFiles/bufferpool.dir/config.cpp.o" \
"CMakeFiles/bufferpool.dir/disk_manager.cpp.o" \
"CMakeFiles/bufferpool.dir/extendible_hash.cpp.o" \
"CMakeFiles/bufferpool.dir/lru_replacer.cpp.o" \
"CMakeFiles/bufferpool.dir/page.cpp.o"

# External object files for target bufferpool
bufferpool_EXTERNAL_OBJECTS =

../lib/libbufferpool.a: src/CMakeFiles/bufferpool.dir/buffer_pool_manager.cpp.o
../lib/libbufferpool.a: src/CMakeFiles/bufferpool.dir/config.cpp.o
../lib/libbufferpool.a: src/CMakeFiles/bufferpool.dir/disk_manager.cpp.o
../lib/libbufferpool.a: src/CMakeFiles/bufferpool.dir/extendible_hash.cpp.o
../lib/libbufferpool.a: src/CMakeFiles/bufferpool.dir/lru_replacer.cpp.o
../lib/libbufferpool.a: src/CMakeFiles/bufferpool.dir/page.cpp.o
../lib/libbufferpool.a: src/CMakeFiles/bufferpool.dir/build.make
../lib/libbufferpool.a: src/CMakeFiles/bufferpool.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/data/zhengyang/data/GraphScope_bufferpool/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Linking CXX static library ../../lib/libbufferpool.a"
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && $(CMAKE_COMMAND) -P CMakeFiles/bufferpool.dir/cmake_clean_target.cmake
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/bufferpool.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/bufferpool.dir/build: ../lib/libbufferpool.a

.PHONY : src/CMakeFiles/bufferpool.dir/build

src/CMakeFiles/bufferpool.dir/clean:
	cd /data/zhengyang/data/GraphScope_bufferpool/build/src && $(CMAKE_COMMAND) -P CMakeFiles/bufferpool.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/bufferpool.dir/clean

src/CMakeFiles/bufferpool.dir/depend:
	cd /data/zhengyang/data/GraphScope_bufferpool/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /data/zhengyang/data/GraphScope_bufferpool /data/zhengyang/data/GraphScope_bufferpool/src /data/zhengyang/data/GraphScope_bufferpool/build /data/zhengyang/data/GraphScope_bufferpool/build/src /data/zhengyang/data/GraphScope_bufferpool/build/src/CMakeFiles/bufferpool.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/bufferpool.dir/depend

