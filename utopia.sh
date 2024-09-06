#!/bin/sh

# SPDX-License-Identifier: Apache-2.0

rlwrap -c -f $UTOPIA_HOME/config/commands $UTOPIA_HOME/build/src/umain "$@"
