#!/bin/sh

# SPDX-License-Identifier: Apache-2.0

# Remove test-generated data
rm -rf ${UTOPIA_HOME}/output

# Remove gcov-generated files before testing
find ${UTOPIA_HOME}/build -type f -name "*.gcda" -o -name "*.gcno" -delete

${UTOPIA_HOME}/build/test/utest
