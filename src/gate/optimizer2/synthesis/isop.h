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

namespace eda::gate::optimizer2::synthesis {

/// Synthesizes the Subnet by using a concrete algorithm.
template<typename Algorithm>
model::SubnetID launchAlgorithm(const kitty::dynamic_truth_table &func,
                                const Algorithm &algorithm,
                                uint16_t maxArity = -1) {

  model::SubnetBuilder subnetBuilder;
  model::Subnet::LinkList inputs = subnetBuilder.addInputs(func.num_vars());

  bool one{kitty::is_const0(~func)};
  bool zero{ kitty::is_const0(func)};

  model::Subnet::Link output;

  if (one || zero) {
    output = subnetBuilder.addCell(one ? model::ONE : model::ZERO);
  } else {
    output = algorithm.run(func, inputs, subnetBuilder, maxArity);
  }

  subnetBuilder.addOutput(output);

  return subnetBuilder.make();
}

/**
 * \brief Implements method of synthesis, by Minato-Morreale algorithm, which
 * was included from kitty library.
*/
class MMSynthesizer final : public Synthesizer<kitty::dynamic_truth_table> {
public:

  using Cube          = kitty::cube;
  using ISOP          = std::vector<Cube>;
  using KittyTT       = kitty::dynamic_truth_table;
  using Link          = model::Subnet::Link;
  using LinkList      = model::Subnet::LinkList;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetID      = model::SubnetID;

  /// Synthesizes the Subnet.
  SubnetID synthesize(const KittyTT &func, uint16_t maxArity = -1) override {
    return launchAlgorithm<MMSynthesizer>(func, *this, maxArity);
  }

  /// Synthesizes the Subnet for a non-constant function.
  Link run(const KittyTT &func,
           const LinkList &inputs,
           SubnetBuilder &subnetBuilder,
           uint16_t maxArity = -1) const;

  /// Synthesizes the Subnet without output for passed ISOP.
  Link synthFromISOP(const ISOP &cubes,
                     const LinkList &inputs,
                     SubnetBuilder &subnetBuilder,
                     uint16_t maxArity = -1) const;

private:

  Link synthFromCube(Cube cube,
                     const LinkList &inputs,
                     SubnetBuilder &subnetBuilder,
                     uint16_t maxArity = -1) const;

};

} // namespace eda::gate::optimizer2::synthesis
