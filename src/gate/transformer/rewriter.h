//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer2/cone_builder.h"
#include "gate/optimizer2/resynthesizer.h"

namespace eda::gate::transformer {

/**
 * @brief Finds and applies the best rewritings on each node according to the
 * number of elements in the old and resynthesized cones.
 */
class Rewriter {
public:
  using Subnet = eda::gate::model::Subnet;
  using SubnetID = eda::gate::model::SubnetID;
  using LinkList = Subnet::LinkList;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using CutExtractor = eda::gate::optimizer2::CutExtractor;
  using ConeBuilder = eda::gate::optimizer2::ConeBuilder;
  using ResynthesizerBase = eda::gate::optimizer2::ResynthesizerBase;

  /**
   * @brief Rewrites subnet stored in the passed builder. Rewriting is based on
   * cuts with size <= k used by the passed resythesizer.
   *
   * @param builder SubnetBuilder with the subnet to rewrite.
   * @param resynthesizer Resythesizer used to synthesize new cone for each cut.
   * @param k Maximum number of elements in the cut.
   */
  SubnetBuilder &rewrite(
      SubnetBuilder &builder,
      ResynthesizerBase &resynthesizer,
      const unsigned int k) const;

private:
  void rewriteOnNode(
      SubnetBuilder &builder,
      const size_t entryID,
      ResynthesizerBase &resynthesizer,
      CutExtractor &cutExtractor) const;
};

} // namespace eda::gate::transformer
