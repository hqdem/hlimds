# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13)

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

find_package(LpSolve REQUIRED)
find_package(Z3 REQUIRED)
find_package(CTemplate REQUIRED COMPONENTS nothreads)
find_package(Cudd REQUIRED)
find_package(Yosys REQUIRED)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(test)
