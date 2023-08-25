#!/bin/sh
#
# Run tests are specified by the regexp
#
# SPDX-License-Identifier: Apache-2.0

rm -rf $UTOPIA_HOME/output

${UTOPIA_HOME}/build/test/utest --gtest_filter=${1}

