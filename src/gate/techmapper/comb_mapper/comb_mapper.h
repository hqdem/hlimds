//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library.h"
#include "gate/library/sdc_manager.h"
#include "gate/techmapper/comb_mapper/mapping.h"

#include <map>

namespace eda::gate::techmapper {

class CombMapper {
  using SCLibrary = library::SCLibrary;
  using SDC = library::SDC;
public:
  virtual ~CombMapper() = default;
  virtual void map(const SubnetID subnetID,
                   const SCLibrary &cellDB,
                   const SDC &sdc,
                   Mapping &mapping) = 0;
  SubnetID subnetID;
};

} // namespace eda::gate::techmapper
