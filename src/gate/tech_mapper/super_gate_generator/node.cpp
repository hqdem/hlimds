//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/tech_mapper/super_gate_generator/node.h"

#include <kitty/kitty.hpp>
#include <omp.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <thread>
#include <vector>

namespace eda::gate::techMap {
void Node::delaysCalculation() {
  for (unsigned long i = 0; i < inputs.size() &&
                            i < cell->getInputPinsNumber(); i++) {
    double maxInputDelay = inputs.at(i)->getMaxDelay();
    double maxInputPinDelay = cell->getInputPin(i).getMaxdelay();
    delays.push_back(maxInputDelay + maxInputPinDelay);
  }

  std::vector<double>::iterator newMaxDelayIt =
    std::max_element(delays.begin(), delays.end());
  if (newMaxDelayIt != delays.end() && *newMaxDelayIt > maxDelay) {
    maxDelay = *newMaxDelayIt; }
}
} // namespace eda::gate::techMap