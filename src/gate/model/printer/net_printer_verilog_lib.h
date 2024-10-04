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

void printVerilogMajType(std::ostream &out, const CellType &type);
void printVerilogDffType(std::ostream &out, const CellType &type);
void printVerilogSDffType(std::ostream &out, const CellType &type);
void printVerilogADffType(std::ostream &out, const CellType &type);
void printVerilogDffRsType(std::ostream &out, const CellType &type);
void printVerilogDLatchType(std::ostream &out, const CellType &type);
void printVerilogADLatchType(std::ostream &out, const CellType &type);
void printVerilogDLatchRsType(std::ostream &out, const CellType &type);
void printVerilogLatchRsType(std::ostream &out, const CellType &type);

void printVerilogCellType(std::ostream &out, const CellType &type);
void printVerilogLibrary(std::ostream &out);

} // namespace eda::gate::model
