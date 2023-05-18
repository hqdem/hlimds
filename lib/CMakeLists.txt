# SPDX-License-Identifier: Apache-2.0

add_library(CLI INTERFACE)
target_include_directories(CLI SYSTEM INTERFACE CLI11)

add_library(Json INTERFACE)
target_include_directories(Json SYSTEM INTERFACE json)

add_subdirectory(easyloggingpp)
add_subdirectory(minisat)
add_subdirectory(lorina/lib)
add_subdirectory(lorina/include)
add_subdirectory(sqlite3)
