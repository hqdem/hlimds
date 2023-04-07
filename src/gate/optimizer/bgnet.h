//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gate.h"
#include "gate/model/gnet.h"

#include <memory>
#include <vector>

namespace eda::gate::optimizer {

/**
* \brief GNet with input and output bindings.
* \author <a href="mailto:mrpepelulka@gmail.com">Rustamkhan Ramaldanov</a>
*/
struct BoundGNet {
  std::shared_ptr<model::GNet> net;
  std::vector<model::Gate::Id> inputs, outputs;
};

} // namespace eda::gate::optimizer
