//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/encoder/encoder.h"
#include "gate/symexec/symexec.h"

#include <cassert>

using namespace eda::gate::encoder;

namespace eda::gate::symexec {

void SymbolicExecutor::exec(const Netlist &net) {
  _encoder.encode(net, _cycle);
}

void SymbolicExecutor::exec(const Netlist &net, unsigned cycles) {
  for (unsigned i = 0; i < cycles; i++) {
    exec(net);
    tick();
  }
}

} // namespace eda::gate::symexec
