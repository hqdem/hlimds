//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer2/synthesizer.h"
#include "util/assert.h"

#include "kitty/kitty.hpp"

#include <cstdint>
#include <vector>

namespace eda::gate::optimizer2::resynthesis {

/// Synthesizes the Subnet by using a concrete algorithm.
template<typename Algorithm>
model::SubnetID launchAlgorithm(const kitty::dynamic_truth_table &func,
                                const Algorithm &algorithm,
                                uint16_t maxArity = -1) {

  model::SubnetBuilder subnetBuilder;

  std::vector<size_t> inputs;
  for (size_t i = 0; i < func.num_vars(); ++i) {
    inputs.push_back(
        subnetBuilder.addCell(model::IN, model::SubnetBuilder::INPUT));
  }

  uint32_t dummy{0xffffffff};

  bool one{kitty::is_const0(~func)};
  bool zero{ kitty::is_const0(func)};

  model::Subnet::Link output;

  if (one || zero) {
    output = subnetBuilder.addCell(one ? model::ONE : model::ZERO);
  } else {
    output = algorithm.run(func, inputs, dummy, subnetBuilder, maxArity);
  }

  for (; dummy; dummy &= (dummy - 1)) {
    auto idx = std::log2(dummy - (dummy & (dummy - 1)));
    if (idx >= inputs.size()) {
      break;
    }
    subnetBuilder.setDummy(idx);
  }

  subnetBuilder.addCell(model::OUT, output, model::SubnetBuilder::OUTPUT);

  return subnetBuilder.make();
}

/**
 * \brief Implements method of resynthesis, by Minato-Morreale algorithm, which
 * was included from kitty library.
*/
class MinatoMorrealeAlg final : public Synthesizer<kitty::dynamic_truth_table> {
public:

  using Cube          = kitty::cube;
  using Inputs        = std::vector<size_t>;
  using ISOP          = std::vector<Cube>;
  using KittyTT       = kitty::dynamic_truth_table;
  using Link          = model::Subnet::Link;
  using LinkList      = model::Subnet::LinkList;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetID      = model::SubnetID;

  /// Synthesizes the Subnet.
  SubnetID synthesize(const KittyTT &func, uint16_t maxArity = -1) override {
    return launchAlgorithm<MinatoMorrealeAlg>(func, *this, maxArity);
  }

  /// Synthesizes the Subnet for a non-constant function.
  Link run(const KittyTT &func,
           const Inputs &inputs,
           uint32_t &dummy,
           SubnetBuilder &subnetBuilder,
           uint16_t maxArity = -1) const;

  /// Synthesizes the Subnet without output for passed ISOP.
  Link synthFromISOP(const ISOP &cubes,
                     const Inputs &inputs,
                     uint32_t &dummy,
                     SubnetBuilder &subnetBuilder,
                     uint16_t maxArity = -1) const;

private:

  Link synthFromCube(Cube cube,
                     const Inputs &inputs,
                     uint32_t &dummy,
                     SubnetBuilder &subnetBuilder,
                     uint16_t maxArity = -1) const;

};

} // namespace eda::gate::optimizer2::resynthesis
