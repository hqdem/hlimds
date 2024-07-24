
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
#include "util/env.h"

#include <filesystem>
#include <cassert>

using path = std::filesystem::path;

namespace eda::gate::techmapper {

using CellSymbol = model::CellSymbol;
using NetID = model::NetID;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID = model::SubnetID;

const path home = eda::env::getHomePath();
const path techLib = home /
  "test/data/gate/techmapper" /
  "sky130_fd_sc_hd__ff_100C_1v65.lib"; // TODO
const path sdcPath = home /
  "test/data/gate/techmapper" /
  "test.sdc"; // TODO

std::shared_ptr<SubnetBuilder> parseGraphML(const std::string &fileName);
void printVerilog(const SubnetID subnet);
bool checkAllCellsMapped(const SubnetID subnetID);
void checkEQ(const SubnetID origSubnetID, const SubnetID mappedSubnetID);

} // namespace eda::gate::techmapper

