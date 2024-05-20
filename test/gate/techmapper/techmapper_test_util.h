//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/cell.h"
#include "gate/model/subnet.h"
#include "gate/techmapper/techmapper.h"
#include "util/env.h"

#include <filesystem>
#include <cassert>

using path = std::filesystem::path;

namespace eda::gate::techmapper {

using SubnetID = eda::gate::model::SubnetID;

const path home = eda::env::getHomePath();
const std::filesystem::path techLib = home /
  "test/data/gate/techmapper" /
  "sky130_fd_sc_hd__ff_100C_1v65.lib"; // TODO

SubnetID simpleANDMapping(Techmapper::MapperType mapperType);
SubnetID simpleORMapping(Techmapper::MapperType mapperType);
SubnetID graphMLMapping(Techmapper::MapperType mapperType,
                        const std::string &fileName);
SubnetID andNotMapping(Techmapper::MapperType mapperType);
SubnetID randomMapping(Techmapper::MapperType mapperType);
SubnetID notNotAndMapping(Techmapper::MapperType mapperType);
NetID simpleNetMapping(Techmapper::MapperType mapperType);
SubnetID areaRecoveySubnetMapping(Techmapper::MapperType mapperType);

} // namespace eda::gate::techmapper
