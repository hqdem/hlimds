//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gate.h"

#include <iostream>

namespace eda::gate::model {

Gate::List Gate::_storage = []{
  Gate::List storage;

  storage.reserve(1024*1024);
  return storage;
}();

void Gate::setInputs(const SignalList &inputs) {
  removeLinks();
  _inputs.assign(inputs.begin(), inputs.end());
  appendLinks();
}

static std::ostream &operator <<(std::ostream &out, const Gate::SignalList &signals) {
  bool separator = false;
  for (const Gate::Signal &signal: signals) {
    out << (separator ? ", " : "") << signal.event() << "(" << signal.node() << ")";
    separator = true;
  }
  return out;
}

std::ostream &operator <<(std::ostream &out, const Gate &gate) {
  if (gate.isSource()) {
    out << "S{" << gate.id() << "}";
  } else {
    out << "G{" << gate.id() << " <= " << gate.kind() << "(" << gate.inputs() << ")}";
  }
  return out << "[fo=" << gate.fanout() << "]";
}

} // namespace eda::gate::model
