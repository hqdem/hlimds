//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

namespace eda::gate::synthesizer {

inline void extend(model::SubnetBuilder &builder,
                   model::Subnet::LinkList &word,
                   const uint32_t width,
                   const bool signExtend) {
  if (word.size() >= width) return;

  const auto link = (signExtend && !word.empty())
      ? word.back()
      : builder.addCell(model::ZERO);

  while (word.size() < width) {
    word.push_back(link);
  }
}

inline void extendOutput(model::SubnetBuilder &builder,
                         const uint32_t width,
                         const bool signExtend) {
  if (builder.getOutNum() >= width) return;

  const auto link = (signExtend && builder.getOutNum() > 0)
      ? builder.getLink(*builder.rbegin(), 0) /* MSB */
      : builder.addCell(model::ZERO);

  while (builder.getOutNum() < width) {
    builder.addOutput(link);
  }
}

} // namespace eda::gate::synthesizer
