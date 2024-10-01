//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/array.h"
#include "gate/model/celltype.h"
#include "gate/model/link.h"
#include "gate/model/object.h"
#include "gate/model/storage.h"

#include <vector>

namespace eda::gate::model {

class NetBuilder;

//===----------------------------------------------------------------------===//
// Cell
//===----------------------------------------------------------------------===//

/**
 * \brief Cell in a logic network.
 */
class Cell final : public Object<Cell, CellID> {
  friend class Storage<Cell>;
  friend class NetBuilder;

public:
  using LinkList = std::vector<LinkEnd>;

  static constexpr uint16_t MaxFanin  = 0xffff;
  static constexpr uint16_t MaxFanout = 0xffff;

  bool isIn()   const { return typeSID == CELL_TYPE_SID_IN;   }
  bool isOut()  const { return typeSID == CELL_TYPE_SID_OUT;  }
  bool isZero() const { return typeSID == CELL_TYPE_SID_ZERO; }
  bool isOne()  const { return typeSID == CELL_TYPE_SID_ONE;  }
  bool isBuf()  const { return typeSID == CELL_TYPE_SID_BUF;  }
  bool isNot()  const { return typeSID == CELL_TYPE_SID_NOT;  }
  bool isAnd()  const { return typeSID == CELL_TYPE_SID_AND;  }
  bool isOr()   const { return typeSID == CELL_TYPE_SID_OR;   }
  bool isXor()  const { return typeSID == CELL_TYPE_SID_XOR;  }
  bool isNand() const { return typeSID == CELL_TYPE_SID_NAND; }
  bool isNor()  const { return typeSID == CELL_TYPE_SID_NOR;  }
  bool isXnor() const { return typeSID == CELL_TYPE_SID_XNOR; }
  bool isMaj()  const { return typeSID == CELL_TYPE_SID_MAJ;  }

  /// Returns the full cell type identifier.
  CellTypeID getTypeID() const { return CellTypeID::makeFID(typeSID); }
  /// Returns the reference to the cell type.
  const CellType &getType() const { return CellType::get(getTypeID()); }
  /// Returns the cell symbol.
  CellSymbol getSymbol() const { return getType().getSymbol(); }

  /// Returns the cell fan-in (the number of inputs).
  uint16_t getFanin() const { return fanin; }
  /// Returns the cell fan-out (reference count for all outputs).
  uint16_t getFanout() const { return fanout; }

  /// Returns the input links of the cell.
  LinkList getLinks() const;

  /// Returns the input link of the cell.
  LinkEnd getLink(uint16_t port) const;

private:
  Cell(CellTypeID typeID): typeSID(typeID.getSID()) {}

  Cell(CellTypeID typeID, const LinkList &links);

  /// Set the input link (used by NetBuilder).
  void setLink(uint16_t port, const LinkEnd &source);

  /// Cell type SID.
  const uint32_t typeSID;

  uint16_t fanin{0};
  uint16_t fanout{0};

  /// Links in the external list.
  ArrayID arrayID{OBJ_NULL_ID};
};

static_assert(sizeof(Cell) == CellID::Size);

//===----------------------------------------------------------------------===//
// Cell Builder
//===----------------------------------------------------------------------===//

inline CellID makeCell(CellTypeID typeID) {
  assert(typeID != OBJ_NULL_ID);
  const auto cellID = allocateObject<Cell>(typeID);
  assert(cellID != OBJ_NULL_ID);
  return cellID;
}

inline CellID makeCell(CellTypeID typeID, const Cell::LinkList &links) {
  assert(typeID != OBJ_NULL_ID);
  const auto cellID = allocateObject<Cell>(typeID, links);
  assert(cellID != OBJ_NULL_ID);
  return cellID;
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
