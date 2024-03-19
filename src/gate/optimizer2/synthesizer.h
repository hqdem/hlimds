//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include "cudd.h"
#include "gate/model2/object.h"
#include "kitty/dynamic_truth_table.hpp"

#define CONST_CHECK(func)\
  bool one{kitty::is_const0(~func)};\
  bool zero{kitty::is_const0(func)};\
  if (one || zero) {\
    return synthConstFunc(func.num_vars(), one);\
  }\

namespace eda::gate::optimizer2 {

/**
 * \brief Common interface for synthesizers.
 */
template<typename IR>
class Synthesizer {
public:
  using SubnetID = eda::gate::model::SubnetID;

  Synthesizer() {}
  virtual ~Synthesizer() {}

  virtual SubnetID synthesize(const IR &ir, uint16_t maxArity = -1) = 0;

  virtual SubnetID synthesizeWithFactoring(const IR &ir,
                                           uint16_t maxArity = -1) {
    assert(false && "The method is not overridden");
    return model::OBJ_NULL_ID;
  }

protected:

  model::SubnetID synthConstFunc(size_t vars, bool one) {
    model::SubnetBuilder subnetBuilder;
    subnetBuilder.addInputs(vars);

    subnetBuilder.addOutput(
        subnetBuilder.addCell(one ? model::ONE : model::ZERO));

    return subnetBuilder.make();
  }
};

} // namespace eda::gate::optimizer2
