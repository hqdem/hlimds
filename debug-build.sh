#!/bin/sh

# Build script that enables extended debug printing functions

# SPDX-License-Identifier: Apache-2.0

rm -rf $UTOPIA_HOME/build

cmake -S . -B build -G Ninja -DUTOPIA_DEBUG=ON
cmake --build build
