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
    explicit Link(uint32_t idx): Link(idx, false) {}
    Link(): Link(0) {}

    Link operator~() const { return Link(idx, out, !inv); }

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
    static constexpr auto FlipFlopBits = 32;
    static constexpr auto ArityBits = 6;
    static constexpr auto RefCountBits = 20;

    static constexpr size_t MaxArity = (1 << ArityBits) - 1;
    static constexpr size_t MaxRefCount = (1 << RefCountBits) - 1;

    static constexpr size_t InPlaceLinks = 5;
    static constexpr size_t InEntryLinks = 8;

    /// Constructs a cell.
    Cell(CellTypeID typeID,
         const LinkList &links,
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

      const auto size = std::min(links.size(), InPlaceLinks);
      for (size_t i = 0; i < size; ++i) {
        link[i] = links[i];
      }
    }

    /// Constructs a non-flip-flop cell.
    Cell(CellTypeID typeID, const LinkList &links):
        Cell(typeID, links, false, 0) {}

    /// Constructs a flip-flop cell.
    Cell(CellTypeID typeID, const LinkList &links, uint32_t flipFlopID):
        Cell(typeID, links, true, flipFlopID) {}

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
    Link link[InPlaceLinks];
  };
  static_assert(sizeof(Cell) == 32);

  /// Generalized entry: a cell or an array of additional links.
  union Entry {
    Entry() {}
    Entry(CellTypeID typeID, const LinkList &links):
        cell(typeID, links) {}
    Entry(CellTypeID typeID, const LinkList &links, uint32_t flipFlopID):
        cell(typeID, links, flipFlopID) {}
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

  Subnet &operator =(const Subnet &r) = delete;
  Subnet(const Subnet &r) = delete;

  /// Returns the number of inputs.
  uint16_t getInNum() const { return nIn; }
  /// Returns the number of outputs.
  uint16_t getOutNum() const { return nOut; }

  /// Returns the i-th input link.
  Link getIn(size_t i) const {
    assert(i < nIn);
    return Link(i, 0, 0);
  }

  /// Returns the i-th output link.
  Link getOut(size_t i) const {
    assert(i < nOut);
    const auto &entries = getEntries();
    return entries[entries.size() - nOut + i].cell.link[0];
  }

  /// Returns the minimum and maximum path lengths.
  std::pair<uint32_t, uint32_t> getPathLength() const;

  /// Returns the overall number of entries including inputs and outputs.
  uint32_t size() const { return nEntry; }

  /// Returns the array of entries.
  Array<Entry> getEntries() const { return Array<Entry>(entries); }

  /// Returns the j-th link of the i-th cell.
  Link getLink(size_t i, size_t j) const {
    const auto &entries = getEntries();
    const auto &cell = entries[i].cell;

    if (j < Cell::InPlaceLinks) {
      return cell.link[j];
    }

    const auto k = j - Cell::InPlaceLinks;
    const auto n = Cell::InEntryLinks;

    return entries[i + 1 + (k / n)].link[k % n];
  }

  /// Returns the links of the i-th cell.
  LinkList getLinks(size_t i) const {
    const auto &entries = getEntries();
    const auto &cell = entries[i].cell;

    LinkList links(cell.arity);

    size_t j = 0;
    for (; j < cell.arity && j < Cell::InPlaceLinks; ++j) {
      links[j] = cell.link[j];
    }

    for (; j < cell.arity; ++j) {
      const auto k = j - Cell::InPlaceLinks;
      const auto n = Cell::InEntryLinks;

      links[j] = entries[i + 1 + (k / n)].link[k % n];
    }

    return links;
  }

private:
  /// Constructs a subnet.
  Subnet(uint16_t nIn, uint16_t nOut, const std::vector<Entry> &entries):
      nIn(nIn), nOut(nOut), nEntry(entries.size()),
      entries(ArrayBlock<Entry>::allocate(entries, true, true)) {}

  /// Number of inputs.
  const uint16_t nIn;
  /// Number of outputs.
  const uint16_t nOut;
  /// Total number of entries.
  const uint32_t nEntry;

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

  Link addInput() {
    return addCell(IN);
  }

  Link addOutput(Link link) {
    return addCell(OUT, link);
  }

  Link addInput(uint32_t flipFlopID) {
    const auto result = addInput();
    auto &cell = entries[result.idx].cell;
    cell.flipFlop = 1;
    cell.flipFlopID = flipFlopID;
    return result;
  }

  Link addOutput(Link link, uint32_t flipFlopID) {
    const auto result = addOutput(link);
    auto &cell = entries[result.idx].cell;
    cell.flipFlop = 1;
    cell.flipFlopID = flipFlopID;
    return result;
  }

  Link addCell(CellTypeID typeID, const LinkList &links);

  /// Adds a cell w/o inputs.
  Link addCell(CellSymbol symbol) {
    return addCell(getCellTypeID(symbol), LinkList{});
  }

  /// Adds a cell w/ the linked inputs.
  Link addCell(CellSymbol symbol, const LinkList &links) {
    return addCell(getCellTypeID(symbol), links);
  }

  /// Adds a single-input cell.
  Link addCell(CellSymbol symbol, Link link) {
    return addCell(symbol, LinkList{link});
  }

  /// Adds a two-inputs cell.
  Link addCell(CellSymbol symbol, Link l1, Link l2) {
    return addCell(symbol, LinkList{l1, l2});
  }

  /// Adds a three-inputs cell.
  Link addCell(CellSymbol symbol, Link l1, Link l2, Link l3) {
    return addCell(symbol, LinkList{l1, l2, l3});
  }

  /// Adds a four-inputs cell.
  Link addCell(CellSymbol symbol, Link l1, Link l2, Link l3, Link l4) {
    return addCell(symbol, LinkList{l1, l2, l3, l4});
  }

  /// Adds a five-inputs cell.
  Link addCell(CellSymbol symbol,
      Link l1, Link l2, Link l3, Link l4, Link l5) {
    return addCell(symbol, LinkList{l1, l2, l3, l4, l5});
  }

  /// Adds the given number of inputs.
  LinkList addInputs(size_t nIn) {
    LinkList result(nIn);
    for (size_t i = 0; i < nIn; ++i) {
      result[i] = Link(addInput());
    }
    return result;
  }

  /// Returns the outputs connected to the given links.
  void addOutputs(const LinkList &links) {
    for (const auto link : links) {
      addOutput(link);
    }
  }

  /// Adds a k-ary tree that implements the given function.
  /// The operation should be regroupable (associative).
  Link addCellTree(CellSymbol symbol, const LinkList &links, uint16_t k);

  /// Adds the subnet and connects it via the specified links.
  /// Does not add the output cells (it should be done explicitly).
  /// Returns the output links.
  LinkList addSubnet(const SubnetID subnetID, const LinkList &links);

  /// Adds the single-output subnet and connects it via the specified links.
  /// Returns the output link.
  Link addSingleOutputSubnet(const SubnetID subnetID, const LinkList &links);

  /// Replaces one output subnet from original subnet with rhs subnet.
  /// rhsToLhs should contain mapping from rhs PIs and PO to original subnet
  /// inputs and output, respectively. Replace can be called only for subnets
  /// with maximum cells arity <= Subnet::Cell::InPlaceLinks.
  void replace(
      const SubnetID rhsID,
      std::unordered_map<size_t, size_t> &rhsToLhs);

  /// Sorts entries in topological order accoring to prev and next.
  void sortEntries();

  SubnetID make() {
    assert(nIn > 0 && nOut > 0 && !entries.empty());
    if (subnetEnd != boundEntryID) {
      sortEntries();
    }
    assert(outsOrderCorrect());
    return allocate<Subnet>(nIn, nOut, std::move(entries));
  }

private:
  /// Finds free entry or creates new one and returns its index.
  size_t allocEntry() {
    if (!emptyEntryIDs.empty()) {
      size_t allocatedID = emptyEntryIDs.back();
      emptyEntryIDs.pop_back();
      return allocatedID;
    }
    entries.resize(entries.size() + 1);
    return entries.size() - 1;
  }

  /// Saves passed entry index as free and maintains topological order.
  void deallocEntry(const size_t entryID) {
    setOrder(getPrev(entryID), getNext(entryID));
    emptyEntryIDs.push_back(entryID);
  }

  /// Returns next entry index in topological order.
  size_t getNext(const size_t entryID) const {
    assert(entryID < entries.size());
    if (entryID == subnetEnd) {
      return boundEntryID;
    }
    return entryID >= next.size() || next[entryID] == commonOrderEntryID ?
           entryID + 1 : next[entryID];
  }

  /// Returns previous entry index in topological order.
  size_t getPrev(const size_t entryID) const {
    assert(entryID < entries.size());
    if (entryID == 0) {
      return boundEntryID;
    }
    return entryID >= prev.size() || prev[entryID] == commonOrderEntryID ?
           entryID - 1 : prev[entryID];
  }

  /// Records that secondID is the next entry ID after firstID in topological
  /// order.
  void setOrder(const size_t firstID, const size_t secondID) {
    if (firstID == boundEntryID && secondID == boundEntryID) {
      return;
    }

    if (secondID != boundEntryID && firstID != boundEntryID) {
      if (firstID == subnetEnd) {
        subnetEnd = secondID;
      }
    }
    if (secondID != boundEntryID && getPrev(secondID) != firstID) {
      if (secondID >= prev.size()) {
        prev.resize(secondID + 1, commonOrderEntryID);
      }
      prev[secondID] = firstID;
    }
    if (firstID != boundEntryID && getNext(firstID) != secondID) {
      if (firstID >= next.size()) {
        next.resize(firstID + 1, commonOrderEntryID);
      }
      next[firstID] = secondID;
    }
  }

  /// Assigns cell on entryID new links and deletes old. The number of new links
  /// must be the same as the number of old links.
  void relinkCell(const size_t entryID, const LinkList &newLinks) {
    auto &cell = entries[entryID].cell;
    assert(cell.arity == newLinks.size());

    size_t j = 0;
    for (; j < cell.arity && j < Subnet::Cell::InPlaceLinks; ++j) {
      cell.link[j] = newLinks[j];
    }

    for (; j < cell.arity; ++j) {
      const auto k = j - Subnet::Cell::InPlaceLinks;
      const auto n = Subnet::Cell::InEntryLinks;
      entries[entryID + 1 + (k / n)].link[k % n] = newLinks[j];
    }
  }

  /// Recursively deletes cell on entryID and fanins with refcount == 0.
  void deleteCell(const size_t entryID);

  /// Replaces cell on entryID with new one and recursively deletes cells with
  /// refcount == 0. Number of links of both old and new cells
  /// must be <= Subnet::Cell::InPlaceLinks.
  Link replaceCell(
      const size_t entryID,
      const CellTypeID typeID,
      const LinkList &links);

  /// Checks if the outputs order is correct. Outputs must be at the and of
  /// the entries array.
  bool outsOrderCorrect() const {
    if (!nOut) {
      return false;
    }
    bool outsVisited = false;

    for (size_t curID = 0; curID < entries.size(); ++curID) {
      const auto &cell = entries[curID].cell;
      const auto typeID = cell.getTypeID();
      if (typeID == CELL_TYPE_ID_OUT) {
        outsVisited = true;
        continue;
      }
      if (outsVisited) {
        return false;
      }
      curID += cell.more;
    }
    return true;
  }

  /// Clears replacements context.
  void clearContext() {
    prev.clear();
    next.clear();
    emptyEntryIDs.clear();
    subnetEnd = boundEntryID;
  }

private:
  static constexpr size_t commonOrderEntryID = -1u;
  static constexpr size_t boundEntryID = -2u;

  uint16_t nIn;
  uint16_t nOut;

  std::vector<Subnet::Entry> entries;

  std::vector<size_t> prev;
  std::vector<size_t> next;
  std::vector<size_t> emptyEntryIDs;

  size_t subnetEnd{boundEntryID};
};

std::ostream &operator <<(std::ostream &out, const Subnet &subnet);

} // namespace eda::gate::model
