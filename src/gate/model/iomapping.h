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

using EntryID = model::EntryID;
using SubnetSz = model::SubnetSz;
using EntryIDList = std::vector<EntryID>;

/**
 * @brief Represents an input/output mapping for replacement.
 */
struct InOutMapping final {
  using Link = model::SubnetLink;
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

  SubnetSz getInNum() const { return inputs.size(); }
  SubnetSz getOutNum() const { return outputs.size(); }

  Link getIn(const SubnetSz i) const { return inputs[i]; }
  Link getOut(const SubnetSz i) const { return outputs[i]; }

  LinkList inputs;
  LinkList outputs;
};

} // namespace eda::gate::model
