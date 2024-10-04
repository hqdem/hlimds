//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet_base.h"

#include <cstdint>
#include <vector>

namespace eda::gate::model {

using EntryID = uint32_t;
using EntryIDList = std::vector<EntryID>;

/**
 * @brief Represents an input/output mapping for replacement.
 */
struct InOutMapping final {
  using Link = model::SubnetLink<EntryID>;
  using LinkList = std::vector<Link>;

  InOutMapping() = default;

  InOutMapping(const EntryIDList &inputIDs, const EntryIDList &outputIDs) {
    inputs.reserve(inputIDs.size());
    outputs.reserve(outputIDs.size());
    for (uint16_t i = 0; i < inputIDs.size(); ++i) {
      inputs.push_back(Link(inputIDs[i]));
    }
    for (uint16_t i = 0; i < outputIDs.size(); ++i) {
      outputs.push_back(Link(outputIDs[i]));
    }
  }

  InOutMapping(const LinkList &inputs, const LinkList &outputs):
      inputs(inputs), outputs(outputs) {}

  uint16_t getInNum() const { return inputs.size(); }
  uint16_t getOutNum() const { return outputs.size(); }

  Link getIn(const uint16_t i) const { return inputs[i]; }
  Link getOut(const uint16_t i) const { return outputs[i]; }

  LinkList inputs;
  LinkList outputs;
};

} // namespace eda::gate::model
