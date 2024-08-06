#!/bin/sh

# SPDX-License-Identifier: Apache-2.0

# Remove test-generated data
rm -rf ${UTOPIA_HOME}/output

${UTOPIA_HOME}/build/test/utest
