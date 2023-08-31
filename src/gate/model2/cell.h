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

#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell
//===----------------------------------------------------------------------===//

class Cell final : public Object<Cell, CellID> {
  friend class Storage<Cell>;

public:
  using LinkList = std::vector<LinkEnd>;

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

  const CellType &getType() const { return CellType::get(getTypeID()); }

  uint16_t getFanin() const { return fanin; }
  uint16_t getFanout() const { return fanout; }

  void setFanin(uint16_t value) { fanin = value; }
  void setFanout(uint16_t value) { fanout = value; }

  LinkList getLinks() const;

private:
  static constexpr size_t InPlaceLinks = 3;

  Cell(CellTypeID typeID):
      typeSID(typeID.getSID()), fanin(0), fanout(0) {}

  Cell(CellTypeID typeID, const LinkList &links);

  /// Cell type SID.
  const uint32_t typeSID;

  uint16_t fanin;
  uint16_t fanout;

  union LinkData {
    LinkData() {}

    /// Links in the external list.
    ListID listID;
    /// In-place links.
    LinkEnd link[InPlaceLinks];
  } data;
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

inline CellID makeCell(CellSymbol symbol) {
  return makeCell(getCellTypeID(symbol));
}

inline CellID makeCell(CellSymbol symbol, const Cell::LinkList &links) {
  return makeCell(getCellTypeID(symbol), links);
}

inline CellID makeCell(CellSymbol symbol, LinkEnd link) {
  return makeCell(symbol, Cell::LinkList{link});
}

inline CellID makeCell(CellSymbol symbol, LinkEnd l1, LinkEnd l2) {
  return makeCell(symbol, Cell::LinkList{l1, l2});
}

inline CellID makeCell(CellSymbol symbol, LinkEnd l1, LinkEnd l2, LinkEnd l3) {
  return makeCell(symbol, Cell::LinkList{l1, l2, l3});
}

inline CellID makeCell(CellSymbol symbol,
    LinkEnd l1, LinkEnd l2, LinkEnd l3, LinkEnd l4) {
  return makeCell(symbol, Cell::LinkList{l1, l2, l3, l4});
}

inline CellID makeCell(CellSymbol symbol,
    LinkEnd l1, LinkEnd l2, LinkEnd l3, LinkEnd l4, LinkEnd l5) {
  return makeCell(symbol, Cell::LinkList{l1, l2, l3, l4, l5});
}

inline CellID makeCell(CellSymbol symbol, CellID cell) {
  return makeCell(symbol, LinkEnd(cell));
}

inline CellID makeCell(CellSymbol symbol, CellID c1, CellID c2) {
  return makeCell(symbol, LinkEnd(c1), LinkEnd(c2));
}

inline CellID makeCell(CellSymbol symbol, CellID c1, CellID c2, CellID c3) {
  return makeCell(symbol, LinkEnd(c1), LinkEnd(c2), LinkEnd(c3));
}

inline CellID makeCell(CellSymbol symbol,
    CellID c1, CellID c2, CellID c3, CellID c4) {
  return makeCell(symbol, LinkEnd(c1), LinkEnd(c2), LinkEnd(c3), LinkEnd(c4));
}

inline CellID makeCell(CellSymbol symbol,
    CellID c1, CellID c2, CellID c3, CellID c4, CellID c5) {
  return makeCell(symbol,
    LinkEnd(c1), LinkEnd(c2), LinkEnd(c3), LinkEnd(c4), LinkEnd(c5));
}

} // namespace eda::gate::model
