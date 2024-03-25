//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/model2/cell.h"
#include "gate/techmapper/techmapper.h"

#include <filesystem>
#include <cassert>

using CellSymbol = eda::gate::model::CellSymbol;
using SubnetID   = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

SubnetID simpleANDMapping(Techmapper::MapperType mapperType);
SubnetID simpleORMapping(Techmapper::MapperType mapperType);
SubnetID graphMLMapping(Techmapper::MapperType mapperType,
                        std::string &fileName);
SubnetID andNotMapping(Techmapper::MapperType mapperType);
SubnetID randomMapping(Techmapper::MapperType mapperType);
NetID simpleNetMapping(Techmapper::MapperType mapperType);

} // namespace eda::gate::tech_optimizer