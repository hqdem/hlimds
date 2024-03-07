# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

project(Utopia VERSION 0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")

OPTION(UTOPIA_DEBUG "Extended debug printing" OFF) # Disabled by default

if(UTOPIA_DEBUG)
  add_definitions(-DUTOPIA_DEBUG)
endif()

set(MAIN_LIBRARY ueda)          # Used by {src,test}/CMakeLists.txt
set(ELPP_LIBRARY easyloggingpp) # Used by {lib,src}/CMakeLists.txt

# Check if UTOPIA_HOME env var is set
if(DEFINED ENV{UTOPIA_HOME})
  message(STATUS "UTOPIA_HOME: $ENV{UTOPIA_HOME}")
else()
  message(FATAL_ERROR "UTOPIA_HOME env var is not set!")
endif()

include(UtopiaFindHelper)
set_local_from_env(CTemplate_ROOT CT_DIR)
set_local_from_env(Z3_ROOT Z3_DIR)
set_local_from_env(Cudd_ROOT CUDD_DIR)
set_local_from_env(CIRCT_DIR CIRCT_DIR)

find_package(LpSolve REQUIRED)
find_package(Z3 REQUIRED)
find_package(CTemplate REQUIRED COMPONENTS nothreads)
find_package(Cudd REQUIRED)
find_package(Yosys REQUIRED)
find_package(STACCATO REQUIRED)

find_package(MLIR REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)
find_package(CIRCT REQUIRED CONFIG)
set_property(TARGET LLVMSupport APPEND
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${LLVM_INCLUDE_DIRS}")
set_property(TARGET MLIRSupport APPEND
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${MLIR_INCLUDE_DIRS}")
set_property(TARGET CIRCTSupport APPEND
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CIRCT_INCLUDE_DIRS}")

message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using CIRCTConfig.cmake in: ${CIRCT_DIR}")

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CIRCT_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CIRCT_DIR}/../../../../cmake/modules")

include(TableGen)
include(AddCIRCT)
include(AddLLVM)
include(AddMLIR)

include_directories(${MLIR_INCLUDE_DIRS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CIRCT_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(test)

# Documentation generation with Doxygen (disabled by default)
OPTION(GEN_DOC "Documentation generation" OFF)

if (NOT GEN_DOC)
  return()
endif()

find_package(Doxygen)

if (DOXYGEN_FOUND)
  execute_process(COMMAND doxygen ${PROJECT_SOURCE_DIR}/doc/Doxyfile)
else ()
  message("Error: can't find doxygen, please install it!")
endif()
