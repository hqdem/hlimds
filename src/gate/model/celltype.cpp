//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/celltype.h"
#include "gate/model/net.h"
#include "gate/model/subnet.h"

#include <cassert>

#define CELL_TYPE(name, symbol, p0, p1, p2, p3, p4, p5, p6, p7, p8, nIn, nOut) \
  const CellTypeID CELL_TYPE_ID(symbol) = makeCellType( \
    symbol, \
    name, \
    CellProperties{p0, p1, p2, p3, p4, p5, p6, p7, p8}, \
    nIn, \
    nOut); \
  const uint32_t CELL_TYPE_SID(symbol) = CELL_TYPE_ID(symbol).getSID();

namespace eda::gate::model {

// Properties: A,B,C,D,E,F,G,H,I
//
// A = Cell
// B = Soft
// C = Combinational
// D = Constant
// E = Identity
// F = Commutative
// G = Associative
// H = Regroupable
// I = Negative

//--------------------------------------------------------------------
//       | Name       | Symbol    | Properties        | #In   | #Out |
//--------------------------------------------------------------------
CELL_TYPE("in",         IN,         1,1,0,0,0,0,0,0,0,  0,       1);
CELL_TYPE("out",        OUT,        1,1,0,0,0,0,0,0,0,  1,       0);
CELL_TYPE("0",          ZERO,       1,1,1,1,0,0,0,0,0,  0,       1);
CELL_TYPE("1",          ONE,        1,1,1,1,0,0,0,0,0,  0,       1);
CELL_TYPE("buf",        BUF,        1,1,1,0,1,0,0,0,0,  1,       1);
CELL_TYPE("not",        NOT,        1,1,1,0,0,0,0,0,1,  1,       1);
CELL_TYPE("and",        AND,        1,1,1,0,0,1,1,1,0,  0xffff,  1);
CELL_TYPE("or",         OR,         1,1,1,0,0,1,1,1,0,  0xffff,  1);
CELL_TYPE("xor",        XOR,        1,1,1,0,0,1,1,1,0,  0xffff,  1);
CELL_TYPE("nand",       NAND,       1,1,1,0,0,1,0,0,1,  0xffff,  1);
CELL_TYPE("nor",        NOR,        1,1,1,0,0,1,0,0,1,  0xffff,  1);
CELL_TYPE("xnor",       XNOR,       1,1,1,0,0,1,1,0,1,  0xffff,  1);
CELL_TYPE("maj",        MAJ,        1,1,1,0,0,1,0,0,0,  0xffff,  1);
CELL_TYPE("latch",      LATCH,      1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("dff_p",      DFF_p,      1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("dff_n",      DFF_n,      1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("sdff_pp0",   sDFF_pp0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_pp1",   sDFF_pp1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_pn0",   sDFF_pn0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_pn1",   sDFF_pn1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_np0",   sDFF_np0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_np1",   sDFF_np1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_nn0",   sDFF_nn0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_nn1",   sDFF_nn1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_pp0",   aDFF_pp0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_pp1",   aDFF_pp1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_pn0",   aDFF_pn0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_pn1",   aDFF_pn1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_np0",   aDFF_np0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_np1",   aDFF_np1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_nn0",   aDFF_nn0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_nn1",   aDFF_nn1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("dffrs_ppp",  DFFrs_ppp,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_ppn",  DFFrs_ppn,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_pnp",  DFFrs_pnp,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_pnn",  DFFrs_pnn,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_npp",  DFFrs_npp,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_npn",  DFFrs_npn,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_nnp",  DFFrs_nnp,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_nnn",  DFFrs_nnn,  1,1,0,0,0,0,0,0,0,  4,       1);
//--------------------------------------------------------------------

const Net &CellType::getNet() const {
  assert(isNet());
  return Net::get(implID);
}

const Subnet &CellType::getSubnet() const {
  assert(isSubnet());
  return Subnet::get(implID);
}

} // namespace eda::gate::model
