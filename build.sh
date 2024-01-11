#!/bin/sh

# SPDX-License-Identifier: Apache-2.0

# Clean previous build
rm -rf $UTOPIA_HOME/build

# Remove outdated gcov-generated files
find ${UTOPIA_HOME}/build -type f -name *.gcda -exec rm {} \;
find ${UTOPIA_HOME}/build -type f -name *.gcno -exec rm {} \;

cmake -S . -B build -G Ninja
cmake --build build
