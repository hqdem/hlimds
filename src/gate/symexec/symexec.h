//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/encoder/context.h"
#include "gate/model/netlist.h"

using namespace eda::gate::encoder;
using namespace eda::gate::model;

namespace eda::gate::symexec {

/**
 * \brief Implements a symbolic executor.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class SymbolicExecutor final {
public:
  SymbolicExecutor(): _cycle(1) {}

  void exec(const Netlist &net);
  void exec(const Gate &gate);

  void tick() { _cycle++; }

  unsigned cycle() const { return _cycle; }
  Context& context() { return _context; }

private:
  unsigned _cycle;
  Context _context;
};

} // namespace eda::gate::symexec
