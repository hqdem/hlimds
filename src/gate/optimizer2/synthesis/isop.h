//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer2/synthesis/algebraic_factor.h"
#include "gate/optimizer2/synthesizer.h"
#include "util/kitty_utils.h"

#include "kitty/kitty.hpp"

#include <vector>

namespace eda::gate::optimizer2::synthesis {

/**
 * \brief Implements method of synthesis, by Minato-Morreale algorithm.
*/
class MMSynthesizer final : public Synthesizer<kitty::dynamic_truth_table> {
public:

  /// @cond ALIASES
  using KittyTT       = kitty::dynamic_truth_table;
  using Link          = eda::gate::model::Subnet::Link;
  using LinkList      = eda::gate::model::Subnet::LinkList;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID      = model::SubnetID;
  /// @endcond

  /// Synthesizes the Subnet.
  SubnetID synthesize(const KittyTT &func,
                      uint16_t maxArity = -1) const override {
    CONST_CHECK(func)
    SubnetBuilder subnetBuilder;
    LinkList ins = subnetBuilder.addInputs(func.num_vars());
    bool inv = (kitty::count_ones(func) > (func.num_bits() / 2));
    Link output = inv ?
        ~utils::synthFromSOP(kitty::isop(~func), ins, subnetBuilder, maxArity) :
        utils::synthFromSOP(kitty::isop(func), ins, subnetBuilder, maxArity);                               
    subnetBuilder.addOutput(output);
    return subnetBuilder.make();
  }

  /// Synthesizes the Subnet with algebraic factoring.
  SubnetID synthesizeWithFactoring(const KittyTT &func,
                                   uint16_t maxArity = -1) const override {
    CONST_CHECK(func)
    bool inv = (kitty::count_ones(func) > (func.num_bits() / 2));
    return factor.getSubnet(
        kitty::isop(inv ? ~func : func), func.num_vars(), maxArity, inv);
  }

private:

  /// For the synthesis of Boolean functions with algebraic factoring.
  AlgebraicFactor factor;
};

} // namespace eda::gate::optimizer2::synthesis
