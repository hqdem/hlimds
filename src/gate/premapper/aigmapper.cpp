//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/aigmapper.h"

namespace eda::gate::premapper {

Gate::Id AigMapper::map(const Gate &oldGate,
                        const GateIdMap &oldToNewGates,
                        GNet &newNet) const {
  // TODO:
  return PreMapper::map(oldGate, oldToNewGates, newNet);
}

} // namespace eda::gate::premapper
