//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/cone_builder.h"
#include "gate/techmapper/comb_mapper/cut_based/cut_based_mapper.h"

namespace eda::gate::techmapper {

using Subnet = eda::gate::model::Subnet;
using Cut = eda::gate::optimizer::CutExtractor::Cut;
using ConeBuilder = eda::gate::optimizer::ConeBuilder;
using Cone = eda::gate::optimizer::ConeBuilder::Cone;

class AreaRecovery : public CutBaseMapper {
  void findBest() override;
  float getMinAreaAndCell(SubnetID &cellTechLib, Cut &cut);
  double calcAreaFlow(Cut &cut, std::vector<double> &representAreaFlow,
                      eda::gate::model::Array<Subnet::Entry> &entries,
                      float minArea);

  double calcDepth(std::vector<double> &depth,
                   eda::gate::model::Array<Subnet::Entry> &entries,
                   uint64_t entryIndex, Cut &cut);
};

} // namespace eda::gate::techmapper
