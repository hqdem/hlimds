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
#include "util/hash.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet
//===----------------------------------------------------------------------===//

/// Combinational subnet consisting of elementary cells (no macroblocks).
class Subnet final : public Object<Subnet, SubnetID> {
  friend class Storage<Subnet>;

public:
  /// Returns the entry/link indices of the j-th link of the i-th entry.
  static std::pair<size_t, size_t> getLinkIndices(size_t i, size_t j) {
    if (j < Cell::InPlaceLinks) {
      return {i, j};
    }

    const auto k = j - Cell::InPlaceLinks;
    const auto n = Cell::InEntryLinks;
    return {i + 1 + (k / n), k % n};
  }

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

    LinkList getInPlaceLinks() const {
      LinkList links(std::min(arity, InPlaceLinks));
      for (size_t i = 0; i < links.size(); ++i) {
        links[i] = link[i];
      }
      return links;
    }

    void incRefCount() {
      assert(refcount < Subnet::Cell::MaxRefCount);
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

  /// Returns the overall number of entries including inputs and outputs.
  uint32_t size() const { return nEntry; }

  /// Returns the number of inputs.
  uint16_t getInNum() const { return nIn; }
  /// Returns the number of outputs.
  uint16_t getOutNum() const { return nOut; }

  /// Returns the j-th link of the i-th cell.
  const Link &getLink(size_t i, size_t j) const;
  /// Returns the links of the i-th cell.
  LinkList getLinks(size_t i) const;

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

  /// Returns the array of entries.
  Array<Entry> getEntries() const { return Array<Entry>(entries); }

  /// Returns the minimum and maximum path lengths.
  std::pair<uint32_t, uint32_t> getPathLength() const;

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

class SubnetBuilder;

/// SubnetBuilder entries bidirectional iterator.
class EntryIterator final {
  friend class SubnetBuilder;

public:
  typedef size_t value_type;
  typedef std::ptrdiff_t difference_type;
  typedef const value_type *pointer;
  typedef const value_type &reference;
  typedef std::bidirectional_iterator_tag iterator_category;

private:
  EntryIterator(const SubnetBuilder *builder, size_t entryID) :
    builder(builder), entry(entryID) {};

public:
  bool operator==(const EntryIterator &other) const {
    return other.builder == builder && other.entry == entry;
  }

  bool operator!=(const EntryIterator &other) const {
    return !(*this == other);
  }

  reference operator*() const;
  pointer operator->() const;
  EntryIterator &operator++();
  EntryIterator operator++(int);
  EntryIterator &operator--();
  EntryIterator operator--(int);

private:
  const SubnetBuilder *builder;
  value_type entry;
};

/// Structural hashing (strashing) key.
struct StrashKey final {
  using Cell = Subnet::Cell;
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;

  static bool isEnabled(CellTypeID cellTypeID, const LinkList &cellLinks) {
    return (cellTypeID != CELL_TYPE_ID_IN)
        && (cellTypeID != CELL_TYPE_ID_OUT)
        && (cellLinks.size() <= Subnet::Cell::InPlaceLinks);
  }

  static bool isEnabled(const Cell &cell) {
    return !cell.isIn()
        && !cell.isOut()
        && cell.arity <= Subnet::Cell::InPlaceLinks;
  }

  StrashKey(): StrashKey(0, LinkList{}) {}

  StrashKey(const Cell &cell): StrashKey(cell.getTypeID(), cell.getInPlaceLinks()) {}

  StrashKey(CellTypeID cellTypeID, const LinkList &cellLinks):
    typeID(cellTypeID.getSID()), arity(cellLinks.size()) {
    assert(isEnabled(cellTypeID, cellLinks));

    for (size_t i = 0; i < cellLinks.size(); ++i) {
      links[i] = cellLinks[i];
    }

    const auto &type = CellType::get(cellTypeID);
    if (type.isCommutative()) {
      std::sort(links, links + arity, [](Link lhs, Link rhs) {
        if (lhs.idx != rhs.idx) { return lhs.idx < rhs.idx; }
        if (lhs.out != rhs.out) { return lhs.out < rhs.out; }
        return lhs.inv < rhs.inv;
      });
    }
  }

  bool operator==(const StrashKey &other) const {
    return typeID == other.typeID && arity == other.arity
        && !memcmp(links, other.links, sizeof(links));
  }

  uint32_t typeID;
  uint16_t arity;
  Link links[Subnet::Cell::InPlaceLinks];
};

} // namespace eda::gate::model

template<>
struct std::hash<eda::gate::model::Subnet::Link> {
  using Link = eda::gate::model::Subnet::Link;

  size_t operator()(const Link &link) const noexcept {
    return (link.idx << 4) | (link.out << 1) | link.inv;
  }
};

template<>
struct std::hash<eda::gate::model::StrashKey> {
  using StrashKey = eda::gate::model::StrashKey;

  size_t operator()(const StrashKey &key) const noexcept {
    size_t h = 0;

    for (size_t i = 0; i < key.arity; ++i) {
      eda::util::hash_combine<>(h, key.links[i]);
    }

    eda::util::hash_combine<>(h, key.arity);
    eda::util::hash_combine<>(h, key.typeID);

    return h;
  }
};

namespace eda::gate::model {

class SubnetBuilder final {
  friend EntryIterator;

public:
  using Cell = Subnet::Cell;
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;

  SubnetBuilder(): nIn(0), nOut(0) {
    const size_t n = 1024*1024; // FIXME
    entries.reserve(n);
    prev.reserve(n);
    next.reserve(n);
    strash.reserve(n);
  }

  /// Returns the constant reference to the i-th entry.
  const Subnet::Entry &getEntry(size_t i) const {
    return entries[i];
  }

  /// Returns the non-constant reference to the i-th entry.
  Subnet::Entry &getEntry(size_t i) {
    return entries[i];
  }

  /// Returns the constant reference to the i-th cell.
  const Subnet::Cell &getCell(size_t i) const {
    return entries[i].cell;
  }

  /// Returns the non-constant reference to the i-th cell.
  Subnet::Cell &getCell(size_t i) {
    return entries[i].cell;
  }

  /// Returns the j-th link of the i-th cell.
  const Link &getLink(size_t i, size_t j) const;
  /// Returns the links of the i-th cell.
  LinkList getLinks(size_t i) const;

  /// Adds an input.
  Link addInput() {
    return addCell(IN);
  }

  /// Adds an output.
  Link addOutput(Link link) {
    return addCell(OUT, link);
  }

  /// Adds a flip-flop-related input.
  Link addInput(uint32_t flipFlopID) {
    const auto result = addInput();
    auto &cell = entries[result.idx].cell;
    cell.flipFlop = 1;
    cell.flipFlopID = flipFlopID;
    return result;
  }

  /// Adds a flip-flop-related output.
  Link addOutput(Link link, uint32_t flipFlopID) {
    const auto result = addOutput(link);
    auto &cell = entries[result.idx].cell;
    cell.flipFlop = 1;
    cell.flipFlopID = flipFlopID;
    return result;
  }

  /// Adds a general-type cell.
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
  LinkList addSubnet(SubnetID subnetID, const LinkList &links);

  /// Adds the single-output subnet and connects it via the specified links.
  /// Returns the output link.
  Link addSingleOutputSubnet(SubnetID subnetID, const LinkList &links);

  /// Replaces a single-output fragment with the given subnet (rhs).
  /// rhsToLhs maps the rhs inputs and output to the subnet boundary cells.
  /// Precondition: cell arities <= Subnet::Cell::InPlaceLinks.
  void replace(SubnetID rhsID, std::unordered_map<size_t, size_t> &rhsToLhs);

  /// Merges the given cells (leaves the first one).
  void mergeCells(size_t entryID, const std::unordered_set<size_t> &otherIDs);

  EntryIterator begin() const {
    return EntryIterator(this, 0);
  }

  EntryIterator end() const {
    return EntryIterator(this, upperBoundID);
  }

  std::reverse_iterator<EntryIterator> rbegin() const {
    return std::make_reverse_iterator(end());
  }

  std::reverse_iterator<EntryIterator> rend() const {
    return std::make_reverse_iterator(begin());
  }

  SubnetID make() {
    assert(/* Constant nets have no inputs */ nOut > 0 && !entries.empty());

    if (subnetEnd != normalOrderID) {
      sortEntries();
    }
    assert(checkInputsOrder() && checkOutputsOrder());

    return allocate<Subnet>(nIn, nOut, std::move(entries));
  }

private:
  /// Return the reference to the j-th link of the given cell.
  Link &getLinkRef(size_t entryID, size_t j);

  /// Allocates an entry and returns its index.
  size_t allocEntry();
  /// Returns an entry of the given type or allocates a new one.
  size_t allocEntry(CellTypeID typeID, const LinkList &links);

  /// Deallocates the given entry.
  void deallocEntry(size_t entryID);

  /// Returns the index of the next entry (topological ordering).
  size_t getNext(size_t entryID) const;

  /// Returns the index of the previous entry (topological ordering).
  size_t getPrev(size_t entryID) const;

  /// Specifies the order between the given entries.
  void setOrder(size_t firstID, size_t secondID);

  /// Assigns the new links to the given cell.
  /// The number of new links must be the same as the number of old links.
  void relinkCell(size_t entryID, const LinkList &newLinks);

  /// Deletes the given cell and the cells from the transitive fanin cone
  /// whose reference counts have become zero (recursively).
  void deleteCell(size_t entryID);

  /// Replaces the given cell w/ the new one and deletes the cells from the
  /// transitive fanin cone whose reference counts have become zero (recursively).
  /// The number of links in both cells must be <= Subnet::Cell::InPlaceLinks.
  Link replaceCell(size_t entryID, CellTypeID typeID, const LinkList &links);

  /// Checks if the inputs are located at the beginning of the array.
  bool checkInputsOrder() const;
  /// Checks if the outputs are located at the end of the array.
  bool checkOutputsOrder() const;

  /// Sorts entries in topological order accoring to prev and next.
  void sortEntries();

  /// Clears the replacement context.
  void clearContext();

private:
  static constexpr size_t normalOrderID = -1u;
  static constexpr size_t lowerBoundID = -2u;
  static constexpr size_t upperBoundID = -3u;

  uint16_t nIn;
  uint16_t nOut;

  std::vector<Subnet::Entry> entries;

  std::vector<size_t> prev;
  std::vector<size_t> next;
  std::vector<size_t> emptyEntryIDs;

  size_t subnetEnd{normalOrderID};

  std::unordered_map<StrashKey, size_t> strash;
};

std::ostream &operator <<(std::ostream &out, const Subnet &subnet);

} // namespace eda::gate::model
