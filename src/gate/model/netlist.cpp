//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/netlist.h"

namespace eda::gate::model {

std::ostream& operator <<(std::ostream &out, const Netlist &netlist) {
  for (const auto gate: netlist.gates()) {
    out << *gate << std::endl;
  }
  return out;
}
 
} // namespace eda::gate::model
