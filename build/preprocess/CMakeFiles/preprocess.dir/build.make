# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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

# Produce verbose output by default.
VERBOSE = 1

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ereny/Projects/integral

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ereny/Projects/integral/build

# Include any dependencies generated for this target.
include preprocess/CMakeFiles/preprocess.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include preprocess/CMakeFiles/preprocess.dir/compiler_depend.make

# Include the progress variables for this target.
include preprocess/CMakeFiles/preprocess.dir/progress.make

# Include the compile flags for this target's objects.
include preprocess/CMakeFiles/preprocess.dir/flags.make

preprocess/CMakeFiles/preprocess.dir/net_processing.cc.o: preprocess/CMakeFiles/preprocess.dir/flags.make
preprocess/CMakeFiles/preprocess.dir/net_processing.cc.o: ../preprocess/net_processing.cc
preprocess/CMakeFiles/preprocess.dir/net_processing.cc.o: preprocess/CMakeFiles/preprocess.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ereny/Projects/integral/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object preprocess/CMakeFiles/preprocess.dir/net_processing.cc.o"
	cd /home/ereny/Projects/integral/build/preprocess && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT preprocess/CMakeFiles/preprocess.dir/net_processing.cc.o -MF CMakeFiles/preprocess.dir/net_processing.cc.o.d -o CMakeFiles/preprocess.dir/net_processing.cc.o -c /home/ereny/Projects/integral/preprocess/net_processing.cc

preprocess/CMakeFiles/preprocess.dir/net_processing.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/preprocess.dir/net_processing.cc.i"
	cd /home/ereny/Projects/integral/build/preprocess && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ereny/Projects/integral/preprocess/net_processing.cc > CMakeFiles/preprocess.dir/net_processing.cc.i

preprocess/CMakeFiles/preprocess.dir/net_processing.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/preprocess.dir/net_processing.cc.s"
	cd /home/ereny/Projects/integral/build/preprocess && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ereny/Projects/integral/preprocess/net_processing.cc -o CMakeFiles/preprocess.dir/net_processing.cc.s

# Object files for target preprocess
preprocess_OBJECTS = \
"CMakeFiles/preprocess.dir/net_processing.cc.o"

# External object files for target preprocess
preprocess_EXTERNAL_OBJECTS =

preprocess/preprocess: preprocess/CMakeFiles/preprocess.dir/net_processing.cc.o
preprocess/preprocess: preprocess/CMakeFiles/preprocess.dir/build.make
preprocess/preprocess: preprocess/CMakeFiles/preprocess.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ereny/Projects/integral/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable preprocess"
	cd /home/ereny/Projects/integral/build/preprocess && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/preprocess.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
preprocess/CMakeFiles/preprocess.dir/build: preprocess/preprocess
.PHONY : preprocess/CMakeFiles/preprocess.dir/build

preprocess/CMakeFiles/preprocess.dir/clean:
	cd /home/ereny/Projects/integral/build/preprocess && $(CMAKE_COMMAND) -P CMakeFiles/preprocess.dir/cmake_clean.cmake
.PHONY : preprocess/CMakeFiles/preprocess.dir/clean

preprocess/CMakeFiles/preprocess.dir/depend:
	cd /home/ereny/Projects/integral/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ereny/Projects/integral /home/ereny/Projects/integral/preprocess /home/ereny/Projects/integral/build /home/ereny/Projects/integral/build/preprocess /home/ereny/Projects/integral/build/preprocess/CMakeFiles/preprocess.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : preprocess/CMakeFiles/preprocess.dir/depend

