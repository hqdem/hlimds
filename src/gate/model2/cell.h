//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/celltype.h"
#include "gate/model2/link.h"
#include "gate/model2/object.h"
#include "gate/model2/storage.h"

#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell
//===----------------------------------------------------------------------===//

class Cell final {
  friend class Storage<Cell>;

public:
  using ID = CellID;
  using LinkList = std::vector<Link>;

  bool isIn()    const { return typeSID == CELL_TYPE_SID_IN;    }
  bool isOut()   const { return typeSID == CELL_TYPE_SID_OUT;   }
  bool isZero()  const { return typeSID == CELL_TYPE_SID_ZERO;  }
  bool isOne()   const { return typeSID == CELL_TYPE_SID_ONE;   }
  bool isNop()   const { return typeSID == CELL_TYPE_SID_NOP;   }
  bool isNot()   const { return typeSID == CELL_TYPE_SID_NOT;   }
  bool isAnd()   const { return typeSID == CELL_TYPE_SID_AND;   }
  bool isOr()    const { return typeSID == CELL_TYPE_SID_OR;    }
  bool isXor()   const { return typeSID == CELL_TYPE_SID_XOR;   }
  bool isNand()  const { return typeSID == CELL_TYPE_SID_NAND;  }
  bool isNor()   const { return typeSID == CELL_TYPE_SID_NOR;   }
  bool isXnor()  const { return typeSID == CELL_TYPE_SID_XNOR;  }
  bool isMaj()   const { return typeSID == CELL_TYPE_SID_MAJ;   }
  bool isLatch() const { return typeSID == CELL_TYPE_SID_LATCH; }
  bool isDff()   const { return typeSID == CELL_TYPE_SID_DFF;   }
  bool isDffRs() const { return typeSID == CELL_TYPE_SID_DFFrs; }

  CellTypeID getTypeID() const { return CellTypeID::makeFID(typeSID); }

  uint16_t getFanin()  const { return fanin;  }
  uint16_t getFanout() const { return fanout; }

  void setFanin(uint16_t value)  { fanin  = value; }
  void setFanout(uint16_t value) { fanout = value; }

private:
  Cell(CellTypeID typeID):
      typeSID(typeID.getSID()), fanin(0), fanout(0) {}

  Cell(CellTypeID typeID, const LinkList &links);

  /// Cell type short identifier.
  const uint32_t typeSID;

  uint16_t fanin;
  uint16_t fanout;

  Link link[3];
};

static_assert(sizeof(Cell) == CellID::Size);

//===----------------------------------------------------------------------===//
// Cell Builder
//===----------------------------------------------------------------------===//

inline CellID makeCell(CellTypeID typeID) {
  return allocate<Cell>(typeID);
}

inline CellID makeCell(CellTypeID typeID, const Cell::LinkList &links) {
  return allocate<Cell>(typeID, links);
}

template<CellSymbol S>
CellID make() {
  return makeCell(getCellTypeID(S));
}

template<CellSymbol S>
CellID make(const Cell::LinkList &links) {
  return makeCell(getCellTypeID(S), links);
}

template<CellSymbol S>
CellID make(Link link) {
  return make<S>(Cell::LinkList{link});
}

template<CellSymbol S>
CellID make(Link l1, Link l2) {
  return make<S>(Cell::LinkList{l1, l2});
}

template<CellSymbol S>
CellID make(Link l1, Link l2, Link l3) {
  return make<S>(Cell::LinkList{l1, l2, l3});
}

template<CellSymbol S>
CellID make(Link l1, Link l2, Link l3, Link l4) {
  return make<S>(Cell::LinkList{l1, l2, l3, l4});
}

template<CellSymbol S>
CellID make(Link l1, Link l2, Link l3, Link l4, Link l5) {
  return make<S>(Cell::LinkList{l1, l2, l3, l4, l5});
}

template<CellSymbol S>
CellID make(CellID cell) {
  return make<S>(Link(cell));
}

template<CellSymbol S>
CellID make(CellID c1, CellID c2) {
  return make<S>(Link(c1), Link(c2));
}

template<CellSymbol S>
CellID make(CellID c1, CellID c2, CellID c3) {
  return make<S>(Link(c1), Link(c2), Link(c3));
}

template<CellSymbol S>
CellID make(CellID c1, CellID c2, CellID c3, CellID c4) {
  return make<S>(Link(c1), Link(c2), Link(c3), Link(c4));
}

template<CellSymbol S>
CellID make(CellID c1, CellID c2, CellID c3, CellID c4, CellID c5) {
  return make<S>(Link(c1), Link(c2), Link(c3), Link(c4), Link(c5));
}

} // namespace eda::gate::model
