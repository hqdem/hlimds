//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
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
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID      = eda::gate::model::SubnetID;
  using Subnet        = eda::gate::model::Subnet;
  /// @endcond

  /// Constructs cone for Cell in SubnetBuilder.
  using ConeConstructor = std::function<SubnetID(SubnetBuilder &, size_t,
                                                 uint16_t, EntryMap &)>;

  /// The Predecate for replacing.
  using ReplacePredicate = std::function<bool(const Effect &)>;

  /// Calculates weigts of SubnetBuilder Cells with input weights. 
  using WeightCalculator = std::function<void(SubnetBuilder &,
                                              const std::vector<float> &)>;

  /// Modifier for replace evaluation.
  using CellWeightModifier = SubnetBuilder::CellWeightModifier;

  /**
   * @brief Constructs a refactorer.
   */
  Refactorer(const std::string &name, const ResynthesizerBase &resynthesizer,
             const ConeConstructor *coneConstructor,
             const uint16_t cutSize, const uint16_t careCutSize,
             const ReplacePredicate *replacePredicate,
             const WeightCalculator *weightCalculator = nullptr,
             const CellWeightModifier *weightModifier = nullptr) :
      SubnetInPlaceTransformer(name),
      resynthesizer(resynthesizer),
      coneConstructor(coneConstructor),
      cutSize(cutSize), careCutSize(careCutSize),
      replacePredicate(replacePredicate),
      weightCalculator(weightCalculator),
      weightModifier(weightModifier) { }

  /*
   * @brief  
   */
  void transform(SubnetBuilder &builder) const override;

private:

  void nodeProcessing(SubnetBuilder &builder, SafePasser &iter) const; 

  const ResynthesizerBase &resynthesizer;
  const ConeConstructor *coneConstructor;
  const uint16_t cutSize;
  const uint16_t careCutSize;
  const ReplacePredicate *replacePredicate;
  const WeightCalculator *weightCalculator;
  const CellWeightModifier *weightModifier;
};

} // namespace eda::gate::optimizer
