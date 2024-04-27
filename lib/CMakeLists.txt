# SPDX-License-Identifier: Apache-2.0

add_library(CLI INTERFACE)
target_include_directories(CLI SYSTEM INTERFACE CLI11)

add_library(Json INTERFACE)
target_include_directories(Json SYSTEM INTERFACE json)

add_library(Kitty INTERFACE)
target_include_directories(Kitty INTERFACE kitty/include)

add_library(Mockturtle INTERFACE)
target_include_directories(Mockturtle SYSTEM INTERFACE mockturtle/include)

add_subdirectory(easyloggingpp)
add_subdirectory(lorina/lib)
add_subdirectory(lorina/include)
add_subdirectory(minisat)
add_subdirectory(mockturtle)
add_subdirectory(sqlite3)
add_subdirectory(tinyxml2)
add_subdirectory(readcells)
