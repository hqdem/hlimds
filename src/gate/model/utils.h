//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/depth_find_visitor.h"
#include "gate/optimizer/walker.h"

namespace eda::gate::model {

Gate::SignalList getNewInputs(const Gate::SignalList &oldInputs,
                              const GNet::GateIdMap &oldToNewGates);

Gate::SignalList getNewInputs(const Gate &oldGate,
                              const GNet::GateIdMap &oldToNewGates,
                              size_t &n0,
                              size_t &n1);

void dump(const GNet &net);

int getNetDepth(const GNet &net);

} // namespace eda::gate::model
