//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gate.h"
#include "gate/model/signal.h"

#include <iostream>

namespace eda::gate::model {

std::ostream& operator <<(std::ostream &out, const Signal &signal) {
  return out << signal.kind() << "(" << signal.gate()->id() << ")";
}

} // namespace eda::gate::model
