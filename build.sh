#!/bin/sh

# SPDX-License-Identifier: Apache-2.0

# Clean previous build
rm -rf $UTOPIA_HOME/build

cmake -S . -B build -G Ninja
cmake --build build
