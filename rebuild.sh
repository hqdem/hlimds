#!/bin/sh

# SPDX-License-Identifier: Apache-2.0

# Remove outdated gcov-generated files
find ${UTOPIA_HOME}/build -type f -name *.gcda -exec rm {} \;
find ${UTOPIA_HOME}/build -type f -name *.gcno -exec rm {} \;

cmake --build build
