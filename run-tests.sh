#!/bin/sh

# SPDX-License-Identifier: Apache-2.0

# Remove test-generated data
rm -rf ${UTOPIA_HOME}/output

# Remove gcov-generated files
find ${UTOPIA_HOME}/build -type f -name *.gcda -exec rm {} \;
find ${UTOPIA_HOME}/build -type f -name *.gcno -exec rm {} \;

${UTOPIA_HOME}/build/test/utest
