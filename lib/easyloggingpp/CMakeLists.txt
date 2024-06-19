# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.1)

set(ELPP_LIBRARY easyloggingpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(${ELPP_LIBRARY} easylogging++.cc)
target_include_directories(${ELPP_LIBRARY} PUBLIC .)
