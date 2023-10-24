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

#include <cstdint>
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
    /// Invertor flag (for invertor graphs, e.g. AIG).
    uint32_t inv : 1;
  };
  static_assert(sizeof(Link) == 4);

  using LinkList = std::vector<Link>;

  /// Cell entry.
  struct Cell final {
    static constexpr size_t InPlaceLinks = 5;
    static constexpr size_t InEntryLinks = 8;
    static constexpr size_t MaxCellArity = (1 << 5) - 1;

    /// Constructs a view to the existing cell.
    Cell(CellTypeID typeID, CellID cellID, const LinkList &links, bool in, bool out):
        cell(CellID::makeSID(cellID)),
        in(in),
        out(out),
        dummy(false),
        arity(links.size()),
        more((links.size() + (InEntryLinks - 1) - InPlaceLinks) / InEntryLinks),
        type(CellTypeID::makeSID(typeID)) {
      assert(links.size() <= MaxCellArity);
      assert(!in || arity == 0);

      const auto size = std::min(links.size(), InPlaceLinks);
      for (size_t i = 0; i < size; ++i) {
        link[i] = links[i];
      }
    }

    /// Constructs a virtual cell not presented in a net.
    Cell(CellTypeID typeID, const LinkList &links, bool in, bool out):
      Cell(typeID, OBJ_NULL_ID, links, in, out) {}

    bool isIn()    const { return in;                          }
    bool isOut()   const { return out;                         }
    bool isDummy() const { return dummy;                       }
    bool isPI()    const { return type == CELL_TYPE_SID_IN;    }
    bool isPO()    const { return type == CELL_TYPE_SID_OUT;   }
    bool isZero()  const { return type == CELL_TYPE_SID_ZERO;  }
    bool isOne()   const { return type == CELL_TYPE_SID_ONE;   }
    bool isBuf()   const { return type == CELL_TYPE_SID_BUF;   }
    bool isNot()   const { return type == CELL_TYPE_SID_NOT;   }
    bool isAnd()   const { return type == CELL_TYPE_SID_AND;   }
    bool isOr()    const { return type == CELL_TYPE_SID_OR;    }
    bool isXor()   const { return type == CELL_TYPE_SID_XOR;   }
    bool isNand()  const { return type == CELL_TYPE_SID_NAND;  }
    bool isNor()   const { return type == CELL_TYPE_SID_NOR;   }
    bool isXnor()  const { return type == CELL_TYPE_SID_XNOR;  }
    bool isMaj()   const { return type == CELL_TYPE_SID_MAJ;   }
    bool isNull()  const { return type == CellTypeID::NullSID; } 

    CellTypeID getTypeID() const { return CellTypeID::makeFID(type); }
    const CellType &getType() const { return CellType::get(getTypeID()); }
    
    /// Cell SID or CellID::NullSID (not connected w/ a design).
    uint64_t cell : CellID::Bits;
    /// Input flag.
    uint64_t in : 1;
    /// Output flag.
    uint64_t out : 1;
    /// Dummy input flag.
    uint64_t dummy : 1;
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
    Entry(CellTypeID typeID, const LinkList &links, bool in, bool out):
        cell(typeID, links, in, out) {}
    Entry(CellTypeID typeID, CellID cellID, const LinkList &links, bool in, bool out):
        cell(typeID, cellID, links, in, out) {}
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

  /// Returns the minimum and maximum path lengths.
  std::pair<uint32_t, uint32_t> getPathLength() const;

  /// Returns the overall number of cells including inputs and outputs.
  uint32_t size() const { return nCell; }

  /// Returns the array of entries.
  Array<Entry> getEntries() const { return Array<Entry>(entries); }

  /// Returns the j-th link of the i-th cell.
  Link getLink(size_t i, size_t j) const {
    const auto cells = getEntries();
    const auto &cell = cells[i].cell;

    if (j < Cell::InPlaceLinks) {
      return cell.link[j];
    }

    const auto k = j - Cell::InPlaceLinks;
    const auto n = Cell::InEntryLinks;

    return cells[i + i + (k / n)].link[k % n];
  }

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

  using Kind = std::pair<bool, bool>;

  static constexpr auto INPUT  = std::make_pair(true,  false);
  static constexpr auto OUTPUT = std::make_pair(false, true);
  static constexpr auto INOUT  = std::make_pair(true,  true);
  static constexpr auto INNER  = std::make_pair(false, false);

  SubnetBuilder(): nIn(0), nOut(0), entries() {}

  size_t addCell(CellTypeID typeID, const LinkList &links, Kind kind = INNER) {
    const auto in  = kind.first;
    const auto out = kind.second;
    const auto idx = entries.size();

    entries.emplace_back(typeID, links, in, out);
    if (in)  nIn++;
    if (out) nOut++;

    const auto InPlaceLinks = Subnet::Cell::InPlaceLinks;
    const auto InEntryLinks = Subnet::Cell::InEntryLinks;
    for (size_t i = InPlaceLinks; i < links.size(); i += InEntryLinks) {
      entries.emplace_back(links, i);
    }

    return idx;
  }

  /// Creates a dummy input (a nonessential variable).
  /// Such inputs are introduced to keep the subnet interface unchanged.
  size_t addDummy() {
    const auto idx = addCell(IN, INPUT);
    entries[idx].cell.dummy = 1;
    return idx;
  }

  void setDummy(size_t idx) {
    assert(entries[idx].cell.in);
    entries[idx].cell.dummy = 1;
  }

  size_t addCell(CellSymbol symbol, Kind kind = INNER) {
    return addCell(getCellTypeID(symbol), LinkList{}, kind);
  }

  size_t addCell(CellSymbol symbol, const LinkList &links, Kind kind = INNER) {
    return addCell(getCellTypeID(symbol), links, kind);
  }

  size_t addCell(CellSymbol symbol, Link link, Kind kind = INNER) {
    return addCell(symbol, LinkList{link}, kind);
  }

  size_t addCell(CellSymbol symbol, Link l1, Link l2, Kind kind = INNER) {
    return addCell(symbol, LinkList{l1, l2}, kind);
  }

  size_t addCell(CellSymbol symbol,
      Link l1, Link l2, Link l3, Kind kind = INNER) {
    return addCell(symbol, LinkList{l1, l2, l3}, kind);
  }

  size_t addCell(CellSymbol symbol,
      Link l1, Link l2, Link l3, Link l4, Kind kind = INNER) {
    return addCell(symbol, LinkList{l1, l2, l3, l4}, kind);
  }

  size_t addCell(CellSymbol symbol,
      Link l1, Link l2, Link l3, Link l4, Link l5, Kind kind = INNER) {
    return addCell(symbol, LinkList{l1, l2, l3, l4, l5}, kind);
  }

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
