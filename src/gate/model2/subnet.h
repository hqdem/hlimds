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

#include <ostream>
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
    Link(): Link(0) {}

    /// Entry index.
    uint32_t idx : 28;
    /// Output port.
    uint32_t out : 3;
    /// Invertor flag.
    uint32_t inv : 1;
  };
  static_assert(sizeof(Link) == 4);

  using LinkList = std::vector<Link>;

  /// Cell entry.
  struct Cell final {
    static constexpr size_t InPlaceLinks = 5;
    static constexpr size_t InEntryLinks = 8;

    /// Constructs a view to the existing cell.
    Cell(CellTypeID typeID, CellID cellID, const LinkList &links):
        cell(CellID::makeSID(cellID)),
        arity(links.size()),
        more((links.size() + (InEntryLinks - 1) - InPlaceLinks) / InEntryLinks),
        type(CellTypeID::makeSID(typeID)) {
      assert(links.size() < (1u << 5));

      const auto size = std::min(links.size(), InPlaceLinks);
      for (size_t i = 0; i < size; ++i) {
        link[i] = links[i];
      }
    }

    /// Constructs a virtual cell not presented in a net.
    Cell(CellTypeID typeID, const LinkList &links):
      Cell(typeID, OBJ_NULL_ID, links) {}

    CellTypeID getTypeID() const { return CellTypeID::makeFID(type); }

    const CellType &getType() const { return CellType::get(getTypeID()); }
    
    /// Cell SID or CellID::NullSID (not connected w/ a design).
    uint64_t cell : CellID::Bits;
    /// Cell arity.
    uint64_t arity : 5;
    /// Number of entries for additional links.
    uint64_t more : 3;
    /// Type SID or CellTypeID::NullSID (undefined cell).
    uint32_t type;
    /// Input links.
    Link link[InPlaceLinks];
  };
  static_assert(sizeof(Cell) == 32);

  /// Generalized entry: a cell or an array of additional links.
  union Entry {
    Entry() {}
    Entry(CellTypeID typeID, const LinkList &links):
        cell(typeID, links) {}
    Entry(CellTypeID typeID, CellID cellID, const LinkList &links):
        cell(typeID, cellID, links) {}
    Entry(const LinkList &links, size_t startWith) {
      const auto size = links.size() - startWith;
      for (size_t i = 0; i < size && i < Cell::InEntryLinks; ++i) {
        link[i] = links[startWith + i];
      }
    }

    Cell cell;
    Link link[Cell::InEntryLinks];
  };
  static_assert(sizeof(Entry) == 32);

  uint16_t getInNum() const { return nIn; }
  uint16_t getOutNum() const { return nOut; }

  uint32_t size() const { return nCell; }

  Array<Entry> getEntries() const { return Array<Entry>(entries); }

private:
  /// Constructs a subnet.
  Subnet(uint16_t nIn, uint16_t nOut, const std::vector<Entry> &entries):
      nIn(nIn), nOut(nOut), nCell(entries.size()),
      entries(ArrayBlock<Entry>::allocate(entries, true, true)) {}

  /// Number of inputs.
  const uint16_t nIn;
  /// Number of outputs.
  const uint16_t nOut;
  /// Total number of cells (including inputs and outputs).
  const uint32_t nCell;

  /// Topologically sorted array of entries.
  const ArrayID entries;
};

static_assert(sizeof(Subnet) == SubnetID::Size);

//===----------------------------------------------------------------------===//
// Subnet Builder
//===----------------------------------------------------------------------===//

class SubnetBuilder final {
public:
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;

  SubnetBuilder(): nIn(0), nOut(0), entries() {}

  size_t addCell(CellTypeID typeID) {
    entries.emplace_back(typeID, LinkList{});
    return entries.size() - 1;
  }

  size_t addCell(CellTypeID typeID, const LinkList &links) {
    const auto index = entries.size();
    entries.emplace_back(typeID, links);

    const auto InPlaceLinks = Subnet::Cell::InPlaceLinks;
    const auto InEntryLinks = Subnet::Cell::InEntryLinks;
    for (size_t i = InPlaceLinks; i < links.size(); i += InEntryLinks) {
      entries.emplace_back(links, i);
    }

    return index;
  }

  size_t addCell(CellSymbol symbol) {
    return addCell(getCellTypeID(symbol));
  }

  size_t addCell(CellSymbol symbol, const LinkList &links) {
    return addCell(getCellTypeID(symbol), links);
  }

  size_t addCell(CellSymbol symbol, Link link) {
    return addCell(symbol, LinkList{link});
  }

  size_t addCell(CellSymbol symbol, Link l1, Link l2) {
    return addCell(symbol, LinkList{l1, l2});
  }

  size_t addCell(CellSymbol symbol, Link l1, Link l2, Link l3) {
    return addCell(symbol, LinkList{l1, l2, l3});
  }

  size_t addCell(CellSymbol symbol,
      Link l1, Link l2, Link l3, Link l4) {
    return addCell(symbol, LinkList{l1, l2, l3, l4});
  }

  size_t addCell(CellSymbol symbol,
      Link l1, Link l2, Link l3, Link l4, Link l5) {
    return addCell(symbol, LinkList{l1, l2, l3, l4, l5});
  }

  void setInNum(size_t num) { nIn = num; }

  void setOutNum(size_t num) { nOut = num; }

  SubnetID make() {
    assert(nIn > 0 && nOut > 0);
    assert(nIn + nOut <= entries.size());
    return allocate<Subnet>(nIn, nOut, std::move(entries));
  }

private:
  uint16_t nIn;
  uint16_t nOut;
  std::vector<Subnet::Entry> entries;
};

std::ostream &operator <<(std::ostream &out, const Subnet &subnet);

} // namespace eda::gate::model
