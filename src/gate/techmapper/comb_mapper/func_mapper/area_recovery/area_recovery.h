//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/cone_builder.h"
#include "gate/techmapper/comb_mapper/func_mapper/func_mapper.h"

namespace eda::gate::techmapper {

using Subnet = model::Subnet;
using CellDB = techmapper::CellDB;
using Cut = optimizer::CutExtractor::Cut;
using ConeBuilder = optimizer::ConeBuilder;
using Cone = optimizer::ConeBuilder::Cone;

class AreaRecovery : public FuncMapper {
  void map(const SubnetID subnetID,
           const CellDB &cellDB,
           const SDC &sdc,
           Mapping &mapping) override;

  float getMinAreaAndCell(SubnetID &cellTechLib, Cut &cut,
                          const CellDB &cellDB) const;

  double calcAreaFlow(Cut &cut, std::vector<double> &representAreaFlow,
                      eda::gate::model::Array<Subnet::Entry> &entries,
                      float minArea);

  double calcDepth(std::vector<double> &depth,
                   eda::gate::model::Array<Subnet::Entry> &entries,
                   uint64_t entryIndex, Cut &cut);
  optimizer::CutExtractor *cutExtractor;
};

} // namespace eda::gate::techmapper
