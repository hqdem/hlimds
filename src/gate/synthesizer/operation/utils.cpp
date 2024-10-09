//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "addition.h"
#include "utils.h"

#include <cassert>
#include <cmath>

namespace eda::gate::synthesizer {

void extend(
    model::SubnetBuilder &builder,
    model::Subnet::LinkList &word,
    const uint32_t width,
    const bool signExtend) {
  if (word.size() >= width) return;

  const auto bit = (signExtend && !word.empty())
      ? word.back()
      : builder.addCell(model::ZERO);

  while (word.size() < width) {
    word.push_back(bit);
  }
}

void extendOutput(
    model::SubnetBuilder &builder,
    const uint32_t width,
    const bool signExtend) {
  if (builder.getOutNum() >= width) return;

  const auto bit = (signExtend && builder.getOutNum() > 0)
      ? builder.getLink(*builder.rbegin(), 0) /* MSB */
      : builder.addCell(model::ZERO);

  while (builder.getOutNum() < width) {
    builder.addOutput(bit);
  }
}

model::Subnet::LinkList twosComplement(
    model::SubnetBuilder &builder,
    const model::Subnet::LinkList &word,
    const uint32_t width,
    const bool signExtend) {
  const auto size = std::min(word.size(), static_cast<size_t>(width));
  assert(size != 0);

  model::Subnet::LinkList inverted(width);

  size_t i = 0;
  for (; i < size; ++i) {
    inverted[i] = ~word[i];
  }

  const auto one = builder.addCell(model::ONE);
  const auto bit = signExtend ? inverted[i - 1] : one;

  for (; i < width; ++i) {
    inverted[i] = bit;
  }

  return synthLadnerFisherAdd(builder, inverted, {one}, width, true);
}

model::Subnet::LinkList absoluteValue(
    model::SubnetBuilder &builder,
    const model::Subnet::LinkList &word) {
  const auto sign = word.back();

  // If sign == 1, construct the twos-complement code.
  // If sign == 0, left the word unchanged.
  model::Subnet::LinkList temp(word.size());
  for (size_t i = 0; i < word.size(); ++i) {
    temp[i] = builder.addCell(model::XOR, word[i], sign);
  }

  return synthLadnerFisherAdd(builder, temp, {sign}, word.size(), true);
}

} // namespace eda::gate::synthesizer
