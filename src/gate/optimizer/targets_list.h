//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"

namespace eda::gate::optimizer {

 /**
  * \brief Class finds and stores a target node of a net and its predecessor.
  */
  class TargetsList {
  public:

    using GNet = eda::gate::model::GNet;
    using Gate = eda::gate::model::Gate;
    using GateId = GNet::GateId;

    TargetsList() = default;

    TargetsList(const GNet *subsNet);

    TargetsList(GateId cutFor);

    bool checkOutGate(const Gate *gate) const;

    size_t getTargetsSize() const;

    const std::vector<GateId> &getTargets() const;

  private:
    // OUT gate, function gate;
    std::vector<GateId> targets;

    void defineTargetLinks(const GNet *subsNet);

    void defineTargetLinks(GateId cutFor);

  };
} // namespace eda::gate::optimizer