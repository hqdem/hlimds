#!/bin/sh

# Build script that enables debug printing.

# SPDX-License-Identifier: Apache-2.0

rm -rf $UTOPIA_HOME/build

cmake -S . -B build -G Ninja \
      -DGEN_DOC=ON \
      -DNPN4_USAGE_STATS=ON \
      -DUTOPIA_DEBUG=ON

cmake --build build
