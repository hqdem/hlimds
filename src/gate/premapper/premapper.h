//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"

#include <memory>
#include <unordered_map>

using namespace eda::gate::model;

namespace eda::gate::premapper {

/**
 * \brief Interface of a pre-mapper, which maps a netlist to the IR (e.g., AIG).
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class PreMapper {
public:
  using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;

  std::shared_ptr<GNet> map(const GNet &net) const;

protected:
  GNet *map(const GNet &net, GateIdMap &oldToNewGates) const;

  /// Creates new gates representing the given one and adds them to the net.
  /// Returns the identifier of the new gate corresponding to the old one or
  /// Gate::INVALID if the operation fails.
  virtual Gate::Id map(const Gate &oldGate,
                       const GateIdMap &oldToNewGates,
                       GNet &newNet) const;
};

} // namespace eda::gate::premapper
