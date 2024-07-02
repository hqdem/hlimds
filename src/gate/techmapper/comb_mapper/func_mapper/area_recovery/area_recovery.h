//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnetview.h"
#include "gate/techmapper/comb_mapper/func_mapper/func_mapper.h"

namespace eda::gate::techmapper {

class AreaRecovery : public FuncMapper {
  using SDC = library::SDC;
  using SCLibrary = library::SCLibrary;
  using Subnet = model::Subnet;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetView = model::SubnetView;
  using Cut = optimizer::CutExtractor::Cut;

  void map(const SubnetID subnetID,
           const SCLibrary &cellDB,
           const SDC &sdc,
           Mapping &mapping) override;

  float getMinAreaAndCell(SubnetID &cellTechLib, Cut &cut,
                          const SCLibrary &cellDB) const;

  double calcAreaFlow(Cut &cut, std::vector<double> &representAreaFlow,
                      eda::gate::model::Array<Subnet::Entry> &entries,
                      float minArea);

  double calcDepth(std::vector<double> &depth,
                   eda::gate::model::Array<Subnet::Entry> &entries,
                   uint64_t entryIndex, Cut &cut);
  optimizer::CutExtractor *cutExtractor;
};

} // namespace eda::gate::techmapper
