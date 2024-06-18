//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include "cudd.h"
#include "kitty/dynamic_truth_table.hpp"
#include "kitty/operations.hpp"
#include "kitty/operators.hpp"

namespace eda::gate::optimizer {

/**
 * \brief Common interface for synthesizers.
 */
template<typename IR>
class Synthesizer {
public:

  using FunctionIR = IR;
  using SubnetID   = eda::gate::model::SubnetID;
  using TruthTable = kitty::dynamic_truth_table;
  
  Synthesizer() {}
  virtual ~Synthesizer() {}

  virtual SubnetID synthesize(const IR &ir, const TruthTable &care,
                              uint16_t maxArity = -1) const = 0;

  SubnetID synthesize(const IR &ir, uint16_t maxArity = -1) const {
    return synthesize(ir, TruthTable(), maxArity);
  }  

  virtual SubnetID synthesizeWithFactoring(const IR &ir, const TruthTable &care,
                                           uint16_t maxArity = -1) const {
    assert(false && "The method is not overridden");
    return model::OBJ_NULL_ID;
  }

  SubnetID synthesizeWithFactoring(const IR &ir, uint16_t maxArity = -1) const {
    return synthesizeWithFactoring(ir, TruthTable(), maxArity);
  }

protected:

  model::SubnetID checkConstTT(TruthTable tt, bool inv) const {
    bool one{kitty::is_const0(~tt)};
    bool zero{kitty::is_const0(tt)};
    if (one || zero) {
      return synthConstFunc(tt.num_vars(), one ^ inv);
    }
    return model::OBJ_NULL_ID;
  }

  model::SubnetID synthConstFunc(size_t vars, bool one) const {
    model::SubnetBuilder subnetBuilder;
    subnetBuilder.addInputs(vars);

    subnetBuilder.addOutput(
        subnetBuilder.addCell(one ? model::ONE : model::ZERO));

    return subnetBuilder.make();
  }
};

/// Truth-table based synthesizer.
using TruthTableSynthesizer = Synthesizer<kitty::dynamic_truth_table>;

} // namespace eda::gate::optimizer
