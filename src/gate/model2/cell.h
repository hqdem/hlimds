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
  bool isBuf()   const { return typeSID == CELL_TYPE_SID_BUF;   }
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

inline CellID makeCell(CellTypeID typeID,
                       const Cell::LinkList &links) {
  return allocate<Cell>(typeID, links);
}

inline CellID makeCell(CellSymbol symbol) {
  return makeCell(getCellTypeID(symbol));
}

inline CellID makeCell(CellSymbol symbol,
                       const Cell::LinkList &links) {
  return makeCell(getCellTypeID(symbol), links);
}

inline CellID makeCell(CellSymbol symbol,
                       Link link) {
  return makeCell(symbol, Cell::LinkList{link});
}

inline CellID makeCell(CellSymbol symbol,
                       Link l1, Link l2) {
  return makeCell(symbol, Cell::LinkList{l1, l2});
}

inline CellID makeCell(CellSymbol symbol,
                       Link l1, Link l2, Link l3) {
  return makeCell(symbol, Cell::LinkList{l1, l2, l3});
}

inline CellID makeCell(CellSymbol symbol,
                       Link l1, Link l2, Link l3, Link l4) {
  return makeCell(symbol, Cell::LinkList{l1, l2, l3, l4});
}

inline CellID makeCell(CellSymbol symbol,
                       Link l1, Link l2, Link l3, Link l4, Link l5) {
  return makeCell(symbol, Cell::LinkList{l1, l2, l3, l4, l5});
}

inline CellID makeCell(CellSymbol symbol,
                       CellID cell) {
  return makeCell(symbol, Link(cell));
}

inline CellID makeCell(CellSymbol symbol,
                       CellID c1, CellID c2) {
  return makeCell(symbol, Link(c1), Link(c2));
}

inline CellID makeCell(CellSymbol symbol,
                       CellID c1, CellID c2, CellID c3) {
  return makeCell(symbol, Link(c1), Link(c2), Link(c3));
}

inline CellID makeCell(CellSymbol symbol,
                       CellID c1, CellID c2, CellID c3, CellID c4) {
  return makeCell(symbol, Link(c1), Link(c2), Link(c3), Link(c4));
}

inline CellID makeCell(CellSymbol symbol,
                       CellID c1, CellID c2, CellID c3, CellID c4, CellID c5) {
  return makeCell(symbol, Link(c1), Link(c2), Link(c3), Link(c4), Link(c5));
}

} // namespace eda::gate::model
