//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/comb_mapper/mapping.h"
#include "gate/techmapper/library/cell_db.h"
#include "gate/techmapper/library/sdc.h"

#include <map>

namespace eda::gate::techmapper {

class CombMapper {
public:
  virtual ~CombMapper() = default;
  virtual void map(const SubnetID subnetID,
                   const CellDB &cellDB,
                   const SDC &sdc,
                   Mapping &mapping) = 0;
  SubnetID subnetID;
};

} // namespace eda::gate::techmapper
