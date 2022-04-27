//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "rtl/model/event.h"

#include <iostream>
#include <vector>

using namespace eda::rtl::model;

namespace eda::gate::model {

/**
 * \brief Represents a triggering signal.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Signal final {
public:
  using GateId = unsigned;
  using List = std::vector<Signal>;

  static Signal posedge(GateId gateId) { return Signal(Event::POSEDGE, gateId); }
  static Signal negedge(GateId gateId) { return Signal(Event::NEGEDGE, gateId); }
  static Signal level0 (GateId gateId) { return Signal(Event::LEVEL0,  gateId); }
  static Signal level1 (GateId gateId) { return Signal(Event::LEVEL1,  gateId); }
  static Signal always (GateId gateId) { return Signal(Event::ALWAYS,  gateId); }

  Signal(Event::Kind kind, GateId gateId):
    _kind(kind), _gateId(gateId) {}

  bool edge() const {
    return _kind == Event::POSEDGE ||
           _kind == Event::NEGEDGE;
  }

  bool level() const {
    return _kind == Event::LEVEL0 ||
           _kind == Event::LEVEL1;
  }

  Event::Kind kind() const { return _kind; }
  GateId gateId() const { return _gateId; }

private:
  Event::Kind _kind;
  GateId _gateId;
};

std::ostream& operator <<(std::ostream &out, const Signal &signal);

} // namespace eda::gate::model
