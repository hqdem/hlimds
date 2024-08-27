//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstdint>
#include <vector>

namespace eda::gate::model {

using EntryID = uint32_t;
using EntryIDList = std::vector<EntryID>;

/**
 * @brief Represents an input/output mapping for replacement.
 */
struct InOutMapping final {
  InOutMapping() = default;

  InOutMapping(const EntryIDList &inputs, const EntryIDList &outputs):
      inputs(inputs), outputs(outputs) {}

  uint16_t getInNum() const { return inputs.size(); }
  uint16_t getOutNum() const { return outputs.size(); }

  EntryID getIn(const uint16_t i) const { return inputs[i]; }
  EntryID getOut(const uint16_t i) const { return outputs[i]; }

  EntryIDList inputs;
  EntryIDList outputs;
};

} // namespace eda::gate::model
