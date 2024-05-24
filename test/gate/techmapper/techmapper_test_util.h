//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/cell.h"
#include "gate/model/celltype.h"
#include "gate/model/subnet.h"
#include "gate/techmapper/techmapper.h"
#include "util/env.h"

#include <filesystem>
#include <cassert>

using path = std::filesystem::path;

namespace eda::gate::techmapper {

using CellSymbol = eda::gate::model::CellSymbol;
using NetID = model::NetID;
using SubnetID = model::SubnetID;
using Strategy = Techmapper::Strategy;

const path home = eda::env::getHomePath();
const path techLib = home /
  "test/data/gate/techmapper" /
  "sky130_fd_sc_hd__ff_100C_1v65.lib"; // TODO

SubnetID simpleANDMapping(const Strategy strategy);
SubnetID simpleORMapping(const Strategy strategy);
SubnetID graphMLMapping(const Strategy strategy,
                        const std::string &fileName);
SubnetID andNotMapping(const Strategy strategy);
SubnetID randomMapping(const Strategy strategy);
SubnetID notNotAndMapping(const Strategy strategy);
NetID simpleNetMapping(const Strategy strategy);
SubnetID areaRecoveySubnetMapping(const Strategy strategy);
SubnetID mapper(const Strategy strategy, const SubnetID subnetId);

} // namespace eda::gate::techmapper
