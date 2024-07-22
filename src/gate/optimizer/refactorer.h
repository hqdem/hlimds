//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnetview.h"
#include "gate/optimizer/resynthesizer.h"
#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/subnet_transformer.h"

#include <functional>
#include <string>

namespace eda::gate::optimizer {

/** 
 * @brief Inplements optimization based on refactoring.
 */
class Refactorer final : public SubnetInPlaceTransformer {
public:

  /// @cond ALIASES
  using Effect        = eda::gate::model::SubnetBuilder::Effect;
  using EntryMap      = std::unordered_map<size_t, size_t>;
  using InOutMapping  = eda::gate::model::InOutMapping;
  using Subnet        = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID      = eda::gate::model::SubnetID;
  using SubnetObject  = eda::gate::model::SubnetObject;
  using SubnetView    = eda::gate::model::SubnetView;
  /// @endcond

  /// Constructs cut for Cell in SubnetBuilder.
  using WindowConstructor =
      std::function<SubnetView(SubnetBuilder &, size_t, uint16_t)>;

  /// The Predecate for replacing.
  using ReplacePredicate = std::function<bool(const Effect &)>;

  /// Calculates weigts of SubnetBuilder Cells with input weights. 
  using WeightCalculator = 
      std::function<void(SubnetBuilder &, const std::vector<float> &)>;

  /// Modifier for replace evaluation.
  using CellWeightModifier = SubnetBuilder::CellWeightModifier;

  /**
   * @brief Constructs a refactorer.
   */
  Refactorer(const std::string &name, const ResynthesizerBase &resynthesizer,
             const WindowConstructor *windowConstructor,
             const uint16_t cutSize, const uint16_t careCutSize,
             const ReplacePredicate *replacePredicate,
             const WeightCalculator *weightCalculator = nullptr,
             const CellWeightModifier *weightModifier = nullptr) :
      SubnetInPlaceTransformer(name),
      resynthesizer(resynthesizer),
      windowConstructor(windowConstructor),
      cutSize(cutSize), careCutSize(careCutSize),
      replacePredicate(replacePredicate),
      weightCalculator(weightCalculator),
      weightModifier(weightModifier) { }

  /*
   * @brief Optimizes the SubnetBuilder.
   */
  void transform(const SubnetBuilderPtr &builder) const override;

private:

  void nodeProcessing(SubnetBuilder &builder, SafePasser &iter) const; 

  const ResynthesizerBase &resynthesizer;
  const WindowConstructor *windowConstructor;
  const uint16_t cutSize;
  const uint16_t careCutSize;
  const ReplacePredicate *replacePredicate;
  const WeightCalculator *weightCalculator;
  const CellWeightModifier *weightModifier;
};

} // namespace eda::gate::optimizer
