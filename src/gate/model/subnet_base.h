//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/celltype.h"
#include "gate/model/object.h"

#include <cstdint>

namespace eda::gate::model {

/// Link source.
template <typename SubnetSize>
struct SubnetLink final {
  using SubnetLinkList = std::vector<SubnetLink<SubnetSize>>;

  SubnetLink(SubnetSize idx, uint8_t out, bool inv):
      idx(idx), out(out), inv(inv) {}

  SubnetLink(SubnetSize idx, bool inv):
      SubnetLink(idx, 0, inv) {}

  explicit SubnetLink(SubnetSize idx):
      SubnetLink(idx, false) {}

  SubnetLink(): SubnetLink(0) {}

  SubnetLink operator~() const {
    return SubnetLink(idx, out, !inv);
  }

  bool operator==(const SubnetLink &other) const {
    return other.idx == idx && other.inv == inv && other.out == out;
  }

  bool operator!=(const SubnetLink &other) const {
    return !(*this == other);
  }

  /// Entry index.
  SubnetSize idx : sizeof(SubnetSize) <= 4 ? 28 : 60;
  /// Output port.
  uint32_t out : 3;
  /// Invertor flag (for invertor graphs, e.g. AIG).
  uint32_t inv : 1;
};

/// Cell entry.
template <typename SubnetSize>
struct SubnetCell final {
  using SubnetLinkList = typename SubnetLink<SubnetSize>::SubnetLinkList;

  static constexpr auto FlipFlopBits = 32;
  static constexpr auto ArityBits = 6;
  static constexpr auto RefCountBits = 20;

  static constexpr uint16_t MaxArity = (1 << ArityBits) - 1;
  static constexpr uint32_t MaxRefCount = (1 << RefCountBits) - 1;

  static constexpr uint16_t InPlaceLinks = 5;
  static constexpr uint16_t InEntryLinks = 8;

  /// Constructs a cell.
  SubnetCell(
    CellTypeID typeID,
    const SubnetLinkList &links,
    bool flipFlop,
    uint32_t flipFlopID):
      flipFlop(flipFlop),
      flipFlopID(flipFlopID),
      arity(links.size()),
      more((links.size() + (InEntryLinks - 1) - InPlaceLinks) / InEntryLinks),
      refcount(0),
      type(CellTypeID::makeSID(typeID)) {
    assert(links.size() <= MaxArity);
    assert((typeID != CELL_TYPE_ID_IN) || arity == 0);

    const uint16_t nLinks = links.size();
    const uint16_t size = std::min(nLinks, InPlaceLinks);
    for (uint16_t i = 0; i < size; ++i) {
      link[i] = links[i];
    }
  }

  /// Constructs a non-flip-flop cell.
  SubnetCell(CellTypeID typeID, const SubnetLinkList &links):
      SubnetCell(typeID, links, false, 0) {}

  /// Constructs a flip-flop cell.
  SubnetCell(
    CellTypeID typeID, const SubnetLinkList &links, uint32_t flipFlopID):
      SubnetCell(typeID, links, true, flipFlopID) {}

  bool isIn()   const { return type == CELL_TYPE_SID_IN;    }
  bool isOut()  const { return type == CELL_TYPE_SID_OUT;   }
  bool isZero() const { return type == CELL_TYPE_SID_ZERO;  }
  bool isOne()  const { return type == CELL_TYPE_SID_ONE;   }
  bool isBuf()  const { return type == CELL_TYPE_SID_BUF;   }
  bool isAnd()  const { return type == CELL_TYPE_SID_AND;   }
  bool isOr()   const { return type == CELL_TYPE_SID_OR;    }
  bool isXor()  const { return type == CELL_TYPE_SID_XOR;   }
  bool isMaj()  const { return type == CELL_TYPE_SID_MAJ;   }
  bool isNull() const { return type == CellTypeID::NullSID; }

  bool isFlipFlop() const { return flipFlop; }

  CellTypeID getTypeID() const { return CellTypeID::makeFID(type); }
  const CellType &getType() const { return CellType::get(getTypeID()); }
  CellSymbol getSymbol() const { return getType().getSymbol(); }

  uint16_t getInNum() const { return arity; }
  uint16_t getOutNum() const { return getType().getOutNum(); }

  SubnetLinkList getInPlaceLinks() const {
    const uint16_t nFanin = arity;
    SubnetLinkList links(std::min(nFanin, InPlaceLinks));
    for (uint16_t i = 0; i < links.size(); ++i) {
      links[i] = link[i];
    }
    return links;
  }

  void incRefCount() {
    assert(refcount < SubnetCell::MaxRefCount);
    refcount++;
  }

  void decRefCount() {
    assert(refcount);
    refcount--;
  }

  /// Flip-flop input/output flag.
  uint64_t flipFlop : 1;
  /// Unique flip-flop identifier (for flip-flip inputs/outputs).
  uint64_t flipFlopID : FlipFlopBits;

  /// Cell arity.
  uint64_t arity : ArityBits;
  /// Number of entries for additional links.
  uint64_t more : 4;

  /// Reference count (fanout).
  uint64_t refcount : RefCountBits;

  /// Type SID or CellTypeID::NullSID (undefined cell).
  uint32_t type;

  /// Input links.
  SubnetLink<SubnetSize> link[InPlaceLinks];
};

/// Generalized entry: a cell or an array of additional links.
template <typename SubnetSize>
union SubnetEntry {
  using SubnetLinkList = typename SubnetLink<SubnetSize>::SubnetLinkList;

  SubnetEntry() {}

  SubnetEntry(CellTypeID typeID, const SubnetLinkList &links):
      cell(typeID, links) {}

  SubnetEntry(
      CellTypeID typeID, const SubnetLinkList &links, uint32_t flipFlopID):
      cell(typeID, links, flipFlopID) {}

  SubnetEntry(const SubnetLinkList &links, uint16_t startWith) {
    const uint16_t size = links.size() - startWith;
    constexpr auto inEntryLinks = SubnetCell<SubnetSize>::InEntryLinks;
    for (uint16_t i = 0; i < size && i < inEntryLinks; ++i) {
      link[i] = links[startWith + i];
    }
  }

  SubnetCell<SubnetSize> cell;
  SubnetLink<SubnetSize> link[SubnetCell<SubnetSize>::InEntryLinks];
};

} // namespace eda::gate::model
