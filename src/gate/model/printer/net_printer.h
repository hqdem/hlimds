//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/net.h"
#include "gate/model/subnet.h"

#include <ostream>

namespace eda::gate::model {

enum Format {
  DEBUG,
  DOT,
  VERILOG,
  LOGDB
};

void print(std::ostream &out,
           const Format format,
           const std::string &name,
           const Net &net,
           const CellTypeID = OBJ_NULL_ID);

void print(std::ostream &out,
           const Format format,
           const std::string &name,
           const Subnet &subnet,
           const CellTypeID = OBJ_NULL_ID);

void print(std::ostream &out,
           const Format format,
           const Net &net,
           const CellTypeID = OBJ_NULL_ID);

void print(std::ostream &out,
           const Format format,
           const Subnet &subnet,
           const CellTypeID = OBJ_NULL_ID);

} // namespace eda::gate::model
