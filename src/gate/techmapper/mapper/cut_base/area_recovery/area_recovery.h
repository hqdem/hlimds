//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/cone_builder.h"
#include "gate/techmapper/mapper/cut_base/cut_base_mapper.h"

namespace eda::gate::tech_optimizer {

using Subnet = eda::gate::model::Subnet;
using Cut = eda::gate::optimizer2::CutExtractor::Cut;
using ConeBuilder = eda::gate::optimizer2::ConeBuilder;
using Cone = eda::gate::optimizer2::ConeBuilder::Cone;

class AreaRecovery : public CutBaseMapper {
  void findBest() override;
  float getMinAreaAndCell(SubnetID &cellTechLib, Cut &cut);
  double calculAreaFlow(Cut &cut, std::vector<double> &representAreaFlow,
                        eda::gate::model::Array<Subnet::Entry> &entries,
                        float minArea);
  double calculDepth(std::vector<double> &depth,
                     eda::gate::model::Array<Subnet::Entry> &entries,
                     uint64_t entryIndex, Cut &cut);
};

} // namespace eda::gate::tech_optimizer