//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_mux.h"

namespace eda::gate::synthesizer {

model::SubnetID synthMux2(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;
  const auto inputs = builder.addInputs(attr.getInWidth());
  const auto &select = inputs[0];

  const auto width = attr.getInWidth(1);
  model::Subnet::LinkList links(width);

  for (uint16_t i = 0; i < width; ++i) {
    const auto &lhs = inputs[1 + i];
    const auto &rhs = inputs[1 + i + width];

    links[i] = builder.addCell(model::OR,
        builder.addCell(model::AND, ~select, lhs),
        builder.addCell(model::AND,  select, rhs));
  }

  builder.addOutputs(links);
  return builder.make();
}

} // namespace eda::gate::synthesizer
