//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/model2/net.h"

#define CELL_TYPE_ID(symbol)  CELL_TYPE_ID_##symbol
#define CELL_TYPE_SID(symbol) CELL_TYPE_SID_##symbol

#define CELL_TYPE(name, symbol, p0, p1, p2, p3, p4, p5, p6, nIn, nOut) \
  const CellTypeID CELL_TYPE_ID(symbol) = makeCellType( \
    name, \
    symbol, \
    CellProperties{p0, p1, p2, p3, p4, p5, p6}, \
    nIn, \
    nOut); \
  const uint32_t CELL_TYPE_SID(symbol) = CELL_TYPE_ID(symbol).getSID();

namespace eda::gate::model {

CELL_TYPE("in",    IN,    0,0,0,0,0,0,0,  0,      1);
CELL_TYPE("out",   OUT,   0,0,0,0,0,0,0,  1,      0);
CELL_TYPE("0",     ZERO,  1,1,0,0,0,0,0,  0,      1);
CELL_TYPE("1",     ONE,   1,1,0,0,0,0,0,  0,      1);
CELL_TYPE("buf",   BUF,   1,0,1,0,0,0,0,  1,      1);
CELL_TYPE("not",   NOT,   1,0,0,0,0,0,1,  1,      1);
CELL_TYPE("and",   AND,   1,0,0,1,1,1,0,  0xffff, 1);
CELL_TYPE("or",    OR,    1,0,0,1,1,1,0,  0xffff, 1);
CELL_TYPE("xor",   XOR,   1,0,0,1,1,1,0,  0xffff, 1);
CELL_TYPE("nand",  NAND,  1,0,0,1,0,0,1,  0xffff, 1);
CELL_TYPE("nor",   NOR,   1,0,0,1,0,0,1,  0xffff, 1);
CELL_TYPE("xnor",  XNOR,  1,0,0,1,1,0,1,  0xffff, 1);
CELL_TYPE("maj",   MAJ,   1,0,0,1,0,0,0,  0xffff, 1);
CELL_TYPE("latch", LATCH, 0,0,0,0,0,0,0,  3,      1);
CELL_TYPE("dff",   DFF,   0,0,0,0,0,0,0,  3,      1);
CELL_TYPE("dffrs", DFFrs, 0,0,0,0,0,0,0,  5,      1);

const Net &CellType::getNet() const {
  return Net::get(netID);
}
const NetID CellType::getNetID() const {
  return netID;
}

} // namespace eda::gate::model
