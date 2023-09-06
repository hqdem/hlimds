//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/array.h"
#include "gate/model2/cell.h"
#include "gate/model2/celltype.h"
#include "gate/model2/object.h"

#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet
//===----------------------------------------------------------------------===//

/// Combinational subnet consisting of elementary cells (no macroblocks).
class Subnet final : public Object<Subnet, SubnetID> {
  friend class Storage<Subnet>;

public:
  /// Link source.
  struct Link final {
    Link(uint32_t idx, uint8_t out, bool inv): idx(idx), out(out), inv(inv) {}
    Link(uint32_t idx, bool inv): Link(idx, 0, inv) {}
    Link(uint32_t idx): Link(idx, false) {}

    /// Entry index.
    uint32_t idx : 28;
    /// Output port.
    uint32_t out : 3;
    /// Invertor flag.
    uint32_t inv : 1;
  };
  static_assert(sizeof(Link) == 4);

  /// Cell entry.
  struct Cell final {
    /// Cell SID or CellID::InvalidSID (not connected w/ a design).
    uint64_t cell : CellID::Bits;
    /// Cell arity.
    uint64_t arity : 5;
    /// Number of entries for additional links.
    uint64_t more : 3;
    /// Type SID or CellTypeID::InvalidSID (undefined cell).
    uint32_t type;
    /// Input links.
    Link link[5];
  };
  static_assert(sizeof(Cell) == 32);

  /// Generalized entry: a cell or an array of additional links.
  union Entry {
    Entry() {} 
    Cell cell;
    Link link[8];
  };
  static_assert(sizeof(Entry) == 32);

  uint16_t getInNumber() const { return nIn; }
  uint16_t getOutNumber() const { return nOut; }

  uint32_t size() const { return nCell; }

  Array<Entry> getEntries() const { return Array<Entry>(entries); }

private:
  /// Constructs a subnet.
  Subnet(uint16_t nIn, uint16_t nOut, const std::vector<Entry> &entries):
      nIn(nIn), nOut(nOut), nCell(entries.size()),
      entries(ArrayBlock<Entry>::allocate(entries, true, true))  {}

  /// Number of inputs.
  const uint16_t nIn;
  /// Number of outputs.
  const uint16_t nOut;
  /// Total number of cells (including inputs and outputs).
  const uint32_t nCell;

  /// Topologically sorted array of entries.
  const ArrayID entries;
};

//===----------------------------------------------------------------------===//
// Subnet Builder
//===----------------------------------------------------------------------===//

class SubnetBuilder final {
public:
  SubnetBuilder() {}

  /*
  void addCell(CellTypeID typeID) {
    return allocate<Cell>(typeID);
  }

  void addCell(CellTypeID typeID, const Cell::LinkList &links) {
    return allocate<Cell>(typeID, links);
  }

  void addCell(CellSymbol symbol) {
    return makeCell(getCellTypeID(symbol));
  }

  void addCell(CellSymbol symbol, const Cell::LinkList &links) {
    return makeCell(getCellTypeID(symbol), links);
  }

  void addCell(CellSymbol symbol, LinkEnd link) {
    return makeCell(symbol, Cell::LinkList{link});
  }

  void addCell(CellSymbol symbol, LinkEnd l1, LinkEnd l2) {
    return makeCell(symbol, Cell::LinkList{l1, l2});
  }

  void addCell(CellSymbol symbol, LinkEnd l1, LinkEnd l2, LinkEnd l3) {
    return makeCell(symbol, Cell::LinkList{l1, l2, l3});
  }

  void addCell(CellSymbol symbol,
      LinkEnd l1, LinkEnd l2, LinkEnd l3, LinkEnd l4) {
    return makeCell(symbol, Cell::LinkList{l1, l2, l3, l4});
  }

  void addCell(CellSymbol symbol,
      LinkEnd l1, LinkEnd l2, LinkEnd l3, LinkEnd l4, LinkEnd l5) {
    return makeCell(symbol, Cell::LinkList{l1, l2, l3, l4, l5});
  }
 */

  // TODO: unique_ptr -> ID.
  //unique_ptr<Subnet> make() {
  //  return std::unique_ptr<Subnet>(new Subnet(std::move(entries)));
  //}
 
private:
  std::vector<Subnet::Entry> entries;
};

} // namespace eda::gate::model
