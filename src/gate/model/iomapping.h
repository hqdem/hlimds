//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>
#include <vector>

namespace eda::gate::model {

/// Represents an input/output mapping for replacement.
struct InOutMapping final {
  InOutMapping() = default;

  InOutMapping(const std::vector<size_t> &inputs,
               const std::vector<size_t> &outputs):
      inputs(inputs), outputs(outputs) {}

  size_t getInNum() const { return inputs.size(); }
  size_t getOutNum() const { return outputs.size(); }

  size_t getIn(const size_t i) const { return inputs[i]; }
  size_t getOut(const size_t i) const { return outputs[i]; }

  std::vector<size_t> inputs;
  std::vector<size_t> outputs;
};

} // namespace eda::gate::model
