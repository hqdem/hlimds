//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnetview.h"
#include "gate/optimizer/conflict_graph.h"
#include "gate/optimizer/resynthesizer.h"
#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/subnet_transformer.h"

#include <functional>
#include <string>

namespace eda::gate::optimizer {

/** 
 * @brief Optimization based on lazy strategy. 
 * 
 * Lazy refactoring with replacement non-intersect subcircuits 
 * after optimization.
 */

class LazyRefactorer final : public SubnetInPlaceTransformer {
public:
  /// @cond ALIASES
  using Effect             = eda::gate::model::SubnetBuilder::Effect;
  using EntryMap           = std::unordered_map<size_t, size_t>;
  using InOutMapping       = eda::gate::model::InOutMapping;
  using Subnet             = eda::gate::model::Subnet;
  using SubnetBuilder      = eda::gate::model::SubnetBuilder;
  using SubnetID           = eda::gate::model::SubnetID;
  using SubnetObject       = eda::gate::model::SubnetObject;
  using SubnetView         = eda::gate::model::SubnetView;
  using CellWeightModifier = SubnetBuilder::CellWeightModifier;
  using SubnetViewWalker   = eda::gate::model::SubnetViewWalker;
  using SubnetBuilderPtr   = eda::gate::optimizer::SubnetBuilderPtr;
  /// @endcond

  using Visitor = std::function<bool(SubnetBuilder &builder,
                                     const bool isIn,
                                     const bool isOut,
                                     const size_t entryID)>;
  
  /// Constructs cone for Cell in SubnetBuilder.
  using ConeConstructor =
      std::function<SubnetView(SubnetBuilder &, size_t root)>;
  
  /// Calculates weigts of SubnetBuilder Cells with input weights. 
  using WeightCalculator = 
      std::function<void(SubnetBuilder &, const std::vector<float> &)>;

  /**
   * @brief Constructs a lazy refactorer.
   *
   * @param name Name of the lazy refactorer.
   * @param resynthesizer Resythesizer used to synthesize new cone.
   * @param coneConstructor Function that build cone for each cell.
   * @param weightCalculator Function that calculates the overall 
   * metric after replacement.
   * @param weightModifier Function that calculates sum weight for each cell.
   */

  LazyRefactorer(const std::string &name,
                 const ResynthesizerBase &resynthesizer,
                 const ConeConstructor *coneConstructor,
                 const WeightCalculator *weightCalculator = nullptr,
                 const CellWeightModifier *weightModifier = nullptr) :
      
    SubnetInPlaceTransformer(name),
    resynthesizer(resynthesizer),
    coneConstructor(coneConstructor),
    weightCalculator(weightCalculator),
    weightModifier(weightModifier) { };

  /**
   * @brief Rewrites the subnet stored in the builder by applying the
   * resynthesizer.
   *
   * @param builder SubnetBuilder with the subnet to rewrite.
   */
  void transform(const SubnetBuilderPtr &builder) const override;

  static SubnetView twoLvlBldr(SubnetBuilder &builder, 
                               size_t numCell);

private:

  void nodeProcessing(SubnetBuilder &builder,
                      EntryIterator &iter, 
                      ConflictGraph &g) const; 

  const ResynthesizerBase &resynthesizer;              
  const ConeConstructor *coneConstructor;
  const WeightCalculator *weightCalculator;
  const CellWeightModifier *weightModifier;

  constexpr static float eps = 1e-7;
};
} // namespace eda::gate::optimizer
