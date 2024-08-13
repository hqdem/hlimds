//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/logger.h"
#include "gate/model/celltype.h"
#include "gate/model/design.h"
#include "gate/model/net.h"
#include "gate/model/subnet.h"

namespace eda::gate::model {

bool validateCellType(const CellType &type, diag::Logger &logger);
bool validateCellType(const CellTypeID typeID, diag::Logger &logger);

bool validateNet(const Net &net, diag::Logger &logger);
bool validateNet(const NetID netID, diag::Logger &logger);

bool validateSubnet(const Subnet &subnet, diag::Logger &logger);
bool validateSubnet(const SubnetBuilder &builder, diag::Logger &logger);
bool validateSubnet(const SubnetID subnetID, diag::Logger &logger);

bool validateDesign(const DesignBuilder &builder, diag::Logger &logger);

} // namespace eda::gate::model
