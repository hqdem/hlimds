//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "gate/model/gate.h"

namespace eda::gate::model {

static std::ostream& operator <<(std::ostream &out, const Signal::List &signals) {
  bool separator = false;
  for (const Signal &signal: signals) {
    out << (separator ? ", " : "") << signal.kind() << "(" << signal.gate()->id() << ")";
    separator = true;
  }
  return out;
}

std::ostream& operator <<(std::ostream &out, const Gate &gate) {
  if (gate.is_source()) {
    return out << "S{" << gate.id() << "}";
  } else {
    return out << "G{" << gate.id() << " <= " << gate.kind() << "(" << gate.inputs() << ")}";
  }
}

} // namespace eda::gate::model
