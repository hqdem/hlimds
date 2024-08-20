//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <gate/model/celltype.h>

#include <ostream>

namespace eda::gate::model {

void printMajType(std::ostream &out, const CellType &type);
void printDffType(std::ostream &out, const CellType &type);
void printSDffType(std::ostream &out, const CellType &type);
void printADffType(std::ostream &out, const CellType &type);
void printDffRsType(std::ostream &out, const CellType &type);
void printDLatchType(std::ostream &out, const CellType &type);
void printADLatchType(std::ostream &out, const CellType &type);
void printDLatchRsType(std::ostream &out, const CellType &type);
void printLatchRsType(std::ostream &out, const CellType &type);

} // namespace eda::gate::model
