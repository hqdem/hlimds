//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/premapper/premapper.h"

namespace eda::gate::premapper {

/**
 * \brief Implements an netlist-to-AIG pre-mapper.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class AigMapper final : public PreMapper {
protected:
  Gate::Id map(const Gate &oldGate,
               const GateIdMap &oldToNewGates,
               GNet &newNet) const override;
};

} // namespace eda::gate::premapper
