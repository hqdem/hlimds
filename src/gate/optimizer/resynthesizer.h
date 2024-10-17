//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/function/bdd.h"
#include "gate/model/subnetview.h"
#include "gate/model/utils/subnetview_to_bdd.h"
#include "gate/optimizer/synthesizer.h"
#include "util/truth_table.h"

#include <cassert>
#include <cstdint>

namespace eda::gate::optimizer {

/**
 * @brief Common interface for subnet-to-subnet resynthesizers.
 */
class ResynthesizerBase {
public:
  ResynthesizerBase() = default;
  virtual ~ResynthesizerBase() = default;

  /**
   * @brief Resynthesizes the subnet view.
   * @return The identifier of the newly constructed subnet or OBJ_NULL_ID.
   */
  virtual model::SubnetObject resynthesize(
      const model::SubnetView &window,
      const uint16_t maxArity = -1) const = 0;
};

/**
 * @brief Constructs the subnet IR.
 */
template<typename IR>
IR construct(const model::SubnetView &window);

/**
 * @brief Subnet-to-subnet resynthesizer based on the IR representation.
 */
template<typename IR>
class Resynthesizer final : public ResynthesizerBase {
public:
  Resynthesizer(const Synthesizer<IR> &synthesizer):
      synthesizer(synthesizer) {}

  model::SubnetObject resynthesize(
      const model::SubnetView &window,
      const uint16_t maxArity = -1) const override {
    const auto ir = construct<IR>(window);
    return synthesizer.synthesize(ir, window.getCare(), maxArity);
  }

private:
  const Synthesizer<IR> &synthesizer;
};

template<>
inline util::TruthTable construct<util::TruthTable>(
    const model::SubnetView &window) {
  return window.evaluateTruthTable();
}

template<>
inline eda::gate::model::SubnetBuilder construct<eda::gate::model::SubnetBuilder>(
    const model::SubnetView &window) {

  const auto &parent = window.getParent();
  if (parent.isNull()) {
    return model::SubnetBuilder();
  }

  auto newW = model::SubnetView(window.getParent().builderPtr(),
                                window.getInOutMapping());
  model::SubnetBuilder newBuilder {newW.getSubnet().make()};
    
  model::SubnetViewWalker walker(window);
  size_t curIn{0};
  auto func = [&newBuilder, &curIn](eda::gate::model::SubnetBuilder &builder,
                                    const bool isIn,
                                    const bool isOut,
                                    const size_t entryID) ->bool {
    if (isIn) {
      newBuilder.setWeight(curIn, builder.getWeight(entryID));
      curIn++;
      return true;
    }
    return false;
  };
  walker.run(func);

  return newBuilder;
}

template<>
inline model::Bdd construct<model::Bdd>(
    const model::SubnetView &window) {
  Cudd *manager = new Cudd(0, 0);
  BDD cuddBdd = eda::gate::model::utils::convertBdd(window, *manager).at(0);
  return {cuddBdd, manager};
}

} // namespace eda::gate::optimizer
