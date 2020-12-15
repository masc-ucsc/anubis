# Find the native LLVM includes and library
#
#  LLVM_INCLUDE_DIR - where to find llvm include files
#  LLVM_LIBRARY_DIR - where to find llvm libs
#  LLVM_CFLAGS      - llvm compiler flags
#  LLVM_LFLAGS      - llvm linker flags
#  LLVM_MODULE_LIBS - list of llvm libs for working with modules.
#  LLVM_FOUND       - True if llvm found.


# First look in ENV{LLVM_HOME} then system path.
find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config llvm-config-3.4
  PATHS
  $ENV{LLVM_HOME}
  NO_DEFAULT_PATH
)
find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config llvm-config-3.4)

find_program(LLVM_CLANG_EXECUTABLE clang++
  PATHS
  $ENV{LLVM_HOME}
  NO_DEFAULT_PATH
)
find_program(LLVM_CLANG_EXECUTABLE clang++)

find_program(LLVM_OPT_EXECUTABLE NAMES opt opt-3.4
  PATHS
  $ENV{LLVM_HOME}
  NO_DEFAULT_PATH
)
find_program(LLVM_OPT_EXECUTABLE NAMES opt opt-3.4)

if (NOT LLVM_CONFIG_EXECUTABLE)
  message(FATAL_ERROR "Could not find llvm-config")
endif (NOT LLVM_CONFIG_EXECUTABLE)

if (NOT LLVM_CLANG_EXECUTABLE)
  message(FATAL_ERROR "Could not find clang++")
endif (NOT LLVM_CLANG_EXECUTABLE)

if (NOT LLVM_OPT_EXECUTABLE)
  message(FATAL_ERROR "Could not find llvm opt")
endif (NOT LLVM_OPT_EXECUTABLE)

message(STATUS "LLVM llvm-config found at: ${LLVM_CONFIG_EXECUTABLE}")
message(STATUS "LLVM clang++ found at: ${LLVM_CLANG_EXECUTABLE}")
message(STATUS "LLVM opt found at: ${LLVM_OPT_EXECUTABLE}")

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
  OUTPUT_VARIABLE LLVM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
  OUTPUT_VARIABLE LLVM_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
  OUTPUT_VARIABLE LLVM_LIBRARY_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# TODO: this does not work well.  the config file will output -I/<include path> and
# also -DNDEBUG.  I've hard coded the #define that are necessary but we should make
# this better.  The necesesary flags are only #defines so maybe just def/undef those
# around #include to llvm headers?
#execute_process(
#  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
#  OUTPUT_VARIABLE LLVM_CFLAGS
#  OUTPUT_STRIP_TRAILING_WHITESPACE
#)
set(LLVM_CFLAGS 
  "-D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
  OUTPUT_VARIABLE LLVM_LFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the link libs we need.  llvm has many and we don't want to link all of the libs
# if we don't need them.   
execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libnames core jit native ipo bitreader target linker
  OUTPUT_VARIABLE LLVM_MODULE_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# CMake really doesn't like adding link directories and wants absolute paths
# Reconstruct it with LLVM_MODULE_LIBS and LLVM_LIBRARY_DIR
string(REPLACE " " ";" LIBS_LIST ${LLVM_MODULE_LIBS})
set (LLVM_MODULE_LIBS "-ldl")
foreach (LIB ${LIBS_LIST})
  set(LLVM_MODULE_LIBS ${LLVM_MODULE_LIBS} "${LLVM_LIBRARY_DIR}/${LIB}")
endforeach(LIB)

message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIR}")
message(STATUS "LLVM lib dir: ${LLVM_LIBRARY_DIR}")
message(STATUS "LLVM libs: ${LLVM_MODULE_LIBS}")
message(STATUS "LLVM compile flags: ${LLVM_CFLAGS}")
