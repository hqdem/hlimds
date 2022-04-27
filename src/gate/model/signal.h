//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gate.h"
#include "rtl/model/event.h"

#include <iostream>
#include <vector>

using namespace eda::rtl::model;

namespace eda::gate::model {

class Gate;

/**
 * \brief Represents a triggering signal.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Signal final {
public:
  using List = std::vector<Signal>;

  Signal(Event::Kind kind, Gate *gate):
    _kind(kind), _gate(gate) {}

  bool edge() const { return _kind == Event::POSEDGE || _kind == Event::NEGEDGE; }
  bool level() const { return _kind == Event::LEVEL0 || _kind == Event::LEVEL1; }

  Event::Kind kind() const { return _kind; }
  Gate* gate() const { return _gate; }

private:
  Event::Kind _kind;
  Gate *_gate;
};

std::ostream& operator <<(std::ostream &out, const Signal &signal);

} // namespace eda::gate::model
