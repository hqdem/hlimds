//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_truth_table.h"
#include "util/kitty_utils.h"

#include "kitty/kitty.hpp"

#include <vector>

namespace eda::gate::optimizer2::synthesis {

/**
 * \brief Implements algebraic factoring for SOPs.
 */
class AlgebraicFactor {
public:

  /// @cond ALIASES
  using Cube          = kitty::cube;
  using Link          = eda::gate::model::Subnet::Link;
  using LinkList      = eda::gate::model::Subnet::LinkList;
  using SOP           = std::vector<Cube>;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID      = eda::gate::model::SubnetID;
  /// @endcond

  /**
   * \brief Synthesizes a subnet from SOP with algebraic factoring.
   * @param func SOP of boolean function.
   * @param funcSize Number of function variables.
   * @param maxArity Max arity of cells.
   * @param inv Inverse function.
   * @return ID of the synthesized subnet.
   */ 
  SubnetID getSubnet(const SOP &func, size_t funcSize, uint16_t maxArity,
                     bool inv = false);

private:

  Link getFactoring(const SOP &func, const LinkList &inputs,
                    SubnetBuilder &subnetBuilder, uint16_t maxArity);

  Link getLiteralFactoring(const SOP &func, const Cube cube,
                           const LinkList &inputs, SubnetBuilder &subnetBuilder,
                           uint16_t maxArity);

  SOP findDivisor(const SOP &func);

  void divide(const SOP &func, const SOP &div, SOP &quo, SOP &rem,
              bool needRem);

  void divideByCube(const SOP &func, const Cube div, SOP &quo, SOP &rem);
};

} // namespace eda::gate::optimizer2::synthesis
