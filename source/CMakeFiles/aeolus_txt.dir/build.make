# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.21

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /root/aeolus

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/aeolus/source

# Include any dependencies generated for this target.
include CMakeFiles/aeolus_txt.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/aeolus_txt.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/aeolus_txt.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/aeolus_txt.dir/flags.make

CMakeFiles/aeolus_txt.dir/tiface.cc.o: CMakeFiles/aeolus_txt.dir/flags.make
CMakeFiles/aeolus_txt.dir/tiface.cc.o: tiface.cc
CMakeFiles/aeolus_txt.dir/tiface.cc.o: CMakeFiles/aeolus_txt.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/aeolus/source/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/aeolus_txt.dir/tiface.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/aeolus_txt.dir/tiface.cc.o -MF CMakeFiles/aeolus_txt.dir/tiface.cc.o.d -o CMakeFiles/aeolus_txt.dir/tiface.cc.o -c /root/aeolus/source/tiface.cc

CMakeFiles/aeolus_txt.dir/tiface.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/aeolus_txt.dir/tiface.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/aeolus/source/tiface.cc > CMakeFiles/aeolus_txt.dir/tiface.cc.i

CMakeFiles/aeolus_txt.dir/tiface.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/aeolus_txt.dir/tiface.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/aeolus/source/tiface.cc -o CMakeFiles/aeolus_txt.dir/tiface.cc.s

# Object files for target aeolus_txt
aeolus_txt_OBJECTS = \
"CMakeFiles/aeolus_txt.dir/tiface.cc.o"

# External object files for target aeolus_txt
aeolus_txt_EXTERNAL_OBJECTS =

aeolus_txt.so: CMakeFiles/aeolus_txt.dir/tiface.cc.o
aeolus_txt.so: CMakeFiles/aeolus_txt.dir/build.make
aeolus_txt.so: /usr/local/lib/libclthreads.so
aeolus_txt.so: CMakeFiles/aeolus_txt.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/aeolus/source/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared module aeolus_txt.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/aeolus_txt.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/aeolus_txt.dir/build: aeolus_txt.so
.PHONY : CMakeFiles/aeolus_txt.dir/build

CMakeFiles/aeolus_txt.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/aeolus_txt.dir/cmake_clean.cmake
.PHONY : CMakeFiles/aeolus_txt.dir/clean

CMakeFiles/aeolus_txt.dir/depend:
	cd /root/aeolus/source && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/aeolus /root/aeolus /root/aeolus/source /root/aeolus/source /root/aeolus/source/CMakeFiles/aeolus_txt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/aeolus_txt.dir/depend

