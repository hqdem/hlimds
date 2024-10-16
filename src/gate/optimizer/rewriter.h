//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnetview.h"
#include "gate/optimizer/cut_extractor.h"
#include "gate/optimizer/resynthesizer.h"
#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/transformer.h"

#include <cstdint>
#include <functional>
#include <string>

namespace eda::gate::optimizer {

/**
 * @brief Finds and applies the best rewritings on each node according to the
 * number of elements in the old and resynthesized cones.
 */
class Rewriter final : public SubnetInPlaceTransformer {
public:
  using InOutMapping = model::InOutMapping;
  using Subnet = model::Subnet;
  using EntryID = model::EntryID;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetObject = model::SubnetObject;
  using SubnetView = model::SubnetView;
  using LinkList = Subnet::LinkList;
  using Effect = SubnetBuilder::Effect;
  using CellActionCallback = SubnetBuilder::CellActionCallback;
  using CellCallbackCondition =
      std::function<void(const EntryID, const uint32_t, const uint32_t)>;

  /**
   * @brief Constructs a rewriter.
   *
   * @param name Name of the rewriter.
   * @param resynthesizer Resythesizer used to synthesize new cone for each cut.
   * @param k Maximum number of elements in the cut.
   * @param cost Function that calculates the overall metric after replacement.
   * A greater returned value is a better result of the replacement.
   * @param zeroCost Enables zero-cost replacements if set.
   */
  Rewriter(
      const std::string &name,
      const ResynthesizerBase &resynthesizer,
      const uint16_t k,
      const std::function<float(const Effect &)> cost,
      const bool zeroCost = false):
    SubnetInPlaceTransformer(name),
    resynthesizer(resynthesizer), k(k), cost(cost), zeroCost(zeroCost) {}

  /**
   * @brief Rewrites the subnet stored in the builder by applying the
   * resynthesizer to k-feasible cuts and choosing the best subnet.
   *
   * @param builder SubnetBuilder with the subnet to rewrite.
   */
  void transform(const std::shared_ptr<SubnetBuilder> &builder) const override;

private:
  void rewriteOnNode(
      const std::shared_ptr<SubnetBuilder> &builder,
      SafePasser &iter,
      CutExtractor &cutExtractor,
      const CellActionCallback *cutRecompute,
      const CellCallbackCondition *cutRecomputeDepthCond) const;

  const ResynthesizerBase &resynthesizer;
  const uint16_t k;
  const std::function<float(const Effect &)> cost;
  const bool zeroCost;

  constexpr static float metricEps = 1e-6;
};

} // namespace eda::gate::optimizer
