//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/array.h"
#include "gate/model/cell.h"
#include "gate/model/celltype.h"
#include "gate/model/object.h"
#include "gate/model/storage.h"
#include "util/hash.h"

#include <algorithm>
#include <cassert>
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
    bool operator==(const Link &other) const {
      return other.idx == idx && other.inv == inv && other.out == out;
    }
    bool operator!=(const Link &other) const {
      return !(*this == other);
    }

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

  /// Returns the i-th input index.
  size_t getInIdx(const size_t i) const { return i; }
  /// Returns the i-th output index.
  size_t getOutIdx(const size_t i) const { return nEntry - nOut + i; }
  /// Returns the maximum entry index.
  size_t getMaxIdx() const { return nEntry - 1; }

  /// Returns the i-th cell.
  const Cell &getCell(size_t i) const;

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

std::ostream &operator <<(std::ostream &out, const Subnet &subnet);

//===----------------------------------------------------------------------===//
// Subnet Builder
//===----------------------------------------------------------------------===//

class SubnetBuilder;

/// SubnetBuilder entries bidirectional iterator.
class EntryIterator {
  friend class SubnetBuilder;

public:
  typedef size_t value_type;
  typedef std::ptrdiff_t difference_type;
  typedef const value_type *pointer;
  typedef const value_type &reference;
  typedef std::bidirectional_iterator_tag iterator_category;

private:
  EntryIterator(const SubnetBuilder *builder, value_type entryID):
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
  EntryIterator next() const;
  EntryIterator &operator--();
  EntryIterator operator--(int);
  EntryIterator prev() const;

protected:
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

  bool operator!=(const StrashKey &other) const {
    return !(*this == other);
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
  using Entry = Subnet::Entry;
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;

  /// Returns the weight of the cell identified by the index.
  using CellWeightProvider = std::function<float(size_t)>;
  /// Calculates the real weight used for replace estimation.
  using CellWeightModifier = std::function<float(float)>;
  /// Performs a certain action in a certain situation.
  using CellActionCallback = std::function<void(size_t)>;

  /// Fanouts container wrapper;
  using FanoutsContainer = std::vector<size_t>;

  /// Represents an input/output mapping for replacement.
  struct InOutMapping final {
    InOutMapping() = default;

    InOutMapping(const std::vector<size_t> &inputs,
                 const std::vector<size_t> &outputs):
        inputs(inputs), outputs(outputs) {}

    size_t getInNum() const { return inputs.size(); }
    size_t getOutNum() const { return outputs.size(); }

    size_t getIn(const size_t i) const { return inputs[i]; }
    size_t getOut(const size_t i) const { return outputs[i]; }

    std::vector<size_t> inputs;
    std::vector<size_t> outputs;
  };

  /// Represents a replacement effect.
  struct Effect final {
    Effect operator+(const Effect &rhs) const {
      return Effect{size + rhs.size, depth + rhs.depth, weight + rhs.weight};
    }

    Effect operator-(const Effect &rhs) const {
      return Effect{size - rhs.size, depth - rhs.depth, weight - rhs.weight};
    }

    /// Change in size: old-size - new-size.
    int size{0};
    /// Change in depth: old-depth - new-depth.
    int depth{0};
    /// Change in weight: old-weight - new-weight.
    float weight{0.};
  };

  static SubnetID makeZero(const size_t nIn);
  static SubnetID makeOne(const size_t nIn);
  static SubnetID makeConst(const size_t nIn, const bool value);

  SubnetBuilder(): nIn(0), nOut(0) {
    const size_t n = 1024; // FIXME
    entries.reserve(n);
    desc.reserve(n);
    depthBounds.reserve(n);
    strash.reserve(n);
  }

  SubnetBuilder(
      const Subnet &subnet,
      const CellWeightProvider *weightProvider = nullptr):
      SubnetBuilder() {
    const auto inputs = addInputs(subnet.getInNum());
    const auto outputs = addSubnet(subnet, inputs, weightProvider);
    addOutputs(outputs);
  }

  SubnetBuilder(
      SubnetID subnetID,
      const CellWeightProvider *weightProvider = nullptr):
      SubnetBuilder(Subnet::get(subnetID), weightProvider) {}

  /// Returns the number of inputs.
  uint16_t getInNum() const { return nIn; }
  /// Returns the number of outputs.
  uint16_t getOutNum() const { return nOut; }

  /// Returns the maximum entry index.
  size_t getMaxIdx() const { return entries.size() - 1; }

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

  /// Returns the depth of the i-th cell.
  size_t getDepth(size_t i) const {
    return desc[i].depth;
  }

  /// Returns the weigth of the i-th cell.
  float getWeight(size_t i) const {
    return desc[i].weight;
  }

  /// Sets the weigth of the i-th cell.
  void setWeight(size_t i, float weight) {
    desc[i].weight = weight;
  }

  /// Returns the pointer to the data associated w/ the i-th cell.
  template <typename T>
  const T *getDataPtr(size_t i) const {
    return static_cast<T*>(desc[i].data);
  }

  /// Sets the pointer to the data associated w/ the i-th cell.
  void setDataPtr(size_t i, const void *data) {
    desc[i].data = const_cast<void*>(data);
  }

  /// Returns the pointer to the data associated w/ the i-th cell.
  template <typename T>
  const T &getDataVal(size_t i) const {
    static_assert(sizeof(T) <= sizeof(void*));
    return reinterpret_cast<const T&>(desc[i].data);
  }

  /// Sets the pointer to the data associated w/ the i-th cell.
  template <typename T>
  void setDataVal(size_t i, const T &data) {
    static_assert(sizeof(T) <= sizeof(void*));
    desc[i].data = reinterpret_cast<void*>(data);
  }

  /// Returns fanouts of the i-th cell.
  FanoutsContainer getFanouts(size_t i) const {
    assert(fanoutsEnabled);
    assert(i < entries.size());
    if (fanouts.size() <= i) {
      return {};
    }
    return fanouts[i];
  }

  /// Returns the entry/link indices of the j-th link of the i-th entry.
  const std::pair<size_t, size_t> getLinkIndices(size_t i, size_t j) const;
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
  LinkList addSubnet(const Subnet &subnet, const LinkList &links,
                     const CellWeightProvider *weightProvider = nullptr);

  LinkList addSubnet(SubnetID subnetID, const LinkList &links,
                     const CellWeightProvider *weightProvider = nullptr) {
    return addSubnet(Subnet::get(subnetID), links, weightProvider);
  }

  /// Adds the single-output subnet and connects it via the specified links.
  /// Returns the output link.
  Link addSingleOutputSubnet(const Subnet &subnet, const LinkList &links);

  Link addSingleOutputSubnet(SubnetID subnetID, const LinkList &links) {
    return addSingleOutputSubnet(Subnet::get(subnetID), links);
  }

  /// Starts a new session.
  /// Precondition: the last started session is ended.
  void startSession() {
    assert(!isSessionStarted);
    isSessionStarted = true;
    ++sessionID;
  }

  /// Ends the current session.
  /// Precondition: session is started.
  void endSession() {
    assert(isSessionStarted);
    isSessionStarted = false;
  }

  /// Marks the given entry (in the current session).
  /// Precondition: session is started.
  void mark(size_t i) {
    assert(isSessionStarted);
    desc[i].session = sessionID;
  }

  /// Checks whether the given entry is marked (in the current session).
  bool isMarked(size_t i) const {
    return desc[i].session == sessionID;
  }

  /// Returns the current or the latest session ID.
  size_t getSessionID() const {
    return sessionID;
  }

  /// Returns the latest session ID in which the given entry was marked.
  /// If the entry has not beent marked during, returns 0.
  size_t getSessionID(size_t i) const {
    return desc[i].session;
  }

  /// Replaces the given single-output fragment w/ the given subnet (rhs).
  /// rhsToLhs maps the rhs inputs and output to the subnet boundary cells.
  /// Precondition: cell arities <= Subnet::Cell::InPlaceLinks.
  void replace(
      const SubnetID rhsID,
      const InOutMapping &iomapping,
      const CellWeightProvider *weightProvider = nullptr,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr);

  /// Replaces the given single-output fragment w/ the given SubnetBuilder (rhs).
  /// rhsToLhs maps the rhs inputs and output to the subnet boundary cells.
  /// Precondition: cell arities <= Subnet::Cell::InPlaceLinks.
  void replace(
      const SubnetBuilder &rhsBuilder,
      const InOutMapping &iomapping,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr);

  /// Returns the effect of the replacement with SubnetID rhs.
  Effect evaluateReplace(
      const SubnetID rhsID,
      const InOutMapping &iomapping,
      const CellWeightProvider *weightProvider = nullptr,
      const CellWeightModifier *weightModifier = nullptr) const;

  /// Returns the effect of the replacement with SubnetBuilder rhs.
  Effect evaluateReplace(
      const SubnetBuilder &rhsBuilder,
      const InOutMapping &iomapping,
      const CellWeightModifier *weightModifier = nullptr) const;

  /// Replaces the given cell w/ the new one. Recursively deletes the cells
  /// from the transitive fanin cone whose reference counts have become zero
  /// (if @param delZeroRefcount is set).
  /// The number of links in both cells must be <= Subnet::Cell::InPlaceLinks.
  Link replaceCell(
      size_t entryID, CellTypeID typeID, const LinkList &links,
      bool delZeroRefcount = true);

  /// Merges the cells from each map item leaving the one stored in the key.
  /// Precondition: remaining entries precede the entries being removed.
  using EntrySet = std::unordered_set<size_t>;
  using MergeMap = std::unordered_map<size_t, EntrySet>;
  void mergeCells(const MergeMap &entryIDs);

  /// Replaces the given cells w/ zero.
  void replaceWithZero(const EntrySet &entryIDs);
  /// Replaces the given cells w/ one.
  void replaceWithOne(const EntrySet &entryIDs);

  /// Enables fanouts receiving by entry index.
  void enableFanouts();

  /// Disables fanouts receiving by entry index.
  void disableFanouts();

  EntryIterator begin() const {
    return EntryIterator(this, getSubnetBegin());
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

  /// @brief Makes a subnet.
  /// @param entryMapping To be filled w/ Subnet entry indices.
  /// @param deleteBufs Indicates if BUFs should be deleted.
  SubnetID make(
      std::vector<size_t> &entryMapping,
      const bool deleteBufs = false) {
    assert(/* Constant nets have no inputs */ nOut > 0 && !entries.empty());

    if (isDisassembled || deleteBufs) {
      rearrangeEntries(entryMapping, deleteBufs);
    }
    assert(checkInputsOrder() && checkOutputsOrder());

    return allocateObject<Subnet>(nIn, nOut, std::move(entries));
  }

  /// @brief Makes a subnet.
  /// @param deleteBufs If set all BUF cells will be deleted.
  SubnetID make(const bool deleteBufs = false) {
    std::vector<size_t> mapping{};
    return make(mapping, deleteBufs);
  }

private:
  /// Template replace method.
  template <typename RhsContainer, typename RhsIterable, typename RhsIt>
  void replace(
      const RhsContainer &rhsContainer,
      const RhsIterable &rhsIterable,
      const size_t rhsOutEntryID,
      const InOutMapping &iomapping,
      const std::function<size_t(RhsIt iter, size_t i)> &getEntryID,
      const CellWeightProvider *weightProvider = nullptr,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr);

  /// Template replace evaluating method.
  template <typename RhsContainer>
  Effect evaluateReplace(
      const RhsContainer &rhsContainer,
      const size_t rhsOutEntryID,
      const InOutMapping &iomapping,
      const CellWeightProvider *weightProvider = nullptr,
      const CellWeightModifier *weightModifier = nullptr) const;

  /// Template new entries evaluating method.
  template <typename RhsContainer, typename RhsIterable, typename RhsIt>
  Effect newEntriesEval(
      const RhsContainer &rhsContainer,
      const RhsIterable &rhsIterable,
      const InOutMapping &iomapping,
      const std::function<size_t(RhsIt iter, size_t i)> &getEntryID,
      std::unordered_set<size_t> &reusedLhsEntries,
      const CellWeightProvider *weightProvider,
      const CellWeightModifier *weightModifier) const;

  /// Returns the add-effect of the replacement:
  /// the number of cells (value of weight) added and new depth of the root.
  Effect newEntriesEval(
      const Subnet &rhs,
      const InOutMapping &iomapping,
      std::unordered_set<size_t> &reusedLhsEntries,
      const CellWeightProvider *weightProvider,
      const CellWeightModifier *weightModifier) const;

  /// Returns the add-effect of the replacement:
  /// the number of cells (value of weight) added and new depth of the root.
  Effect newEntriesEval(
      const SubnetBuilder &rhsBuilder,
      const InOutMapping &iomapping,
      std::unordered_set<size_t> &reusedLhsEntries,
      const CellWeightProvider *weightProvider,
      const CellWeightModifier *weightModifier) const;

  /// Returns the delete-effect of the replacement:
  /// the number of cells (value of weight) deleted.
  Effect deletedEntriesEval(
      const size_t lhsRootEntryID,
      std::unordered_set<size_t> &reusedLhsEntries,
      const CellWeightModifier *weightModifier) const;

  /// Return the reference to the j-th link of the given cell.
  Link &getLinkRef(size_t entryID, size_t j);

  /// Updates deleted entry depth bounds and deletes it from the topological
  /// order.
  void deleteDepthBounds(size_t entryID);

  /// Updates added entry depth bounds and adds it to the topological order.
  void addDepthBounds(size_t entryID);

  /// Adds fanoutID index in the sourceID fanouts storage
  /// (if fanouts storing is enabled).
  void addFanout(size_t sourceID, size_t fanoutID);

  /// Deletes fanoutID index from the sourceID fanouts storage
  /// (if fanouts storing is enabled).
  void delFanout(size_t sourceID, size_t fanoutID);

  /// Allocates an entry and returns its index.
  size_t allocEntry();
  /// Returns an entry of the given type or allocates a new one.
  size_t allocEntry(CellTypeID typeID, const LinkList &links);

  /// Deallocates the given entry.
  void deallocEntry(size_t entryID);

  /// Returns first entry index in topological order.
  size_t getSubnetBegin() const {
    return subnetBegin == normalOrderID ? 0 : subnetBegin;
  }

  /// Returns last entry index in topological order.
  size_t getSubnetEnd() const {
    return subnetEnd == normalOrderID ? entries.size() - 1 : subnetEnd;
  }

  /// Returns the index of the next entry (topological ordering).
  size_t getNext(size_t entryID) const;

  /// Returns the index of the previous entry (topological ordering).
  size_t getPrev(size_t entryID) const;

  /// Specifies the order between the given entries.
  void setOrder(size_t firstID, size_t secondID);

  /// Places the given entry after the pivot.
  /// The given entry should not be in the topological order.
  void placeAfter(size_t entryID, size_t pivotEntryID);

  /// Places the given entry before the pivot.
  /// The given entry should not be in the topological order.
  void placeBefore(size_t entryID, size_t pivotEntryID);

  /// Recursively recomputes the root fanouts depths and updates positions in
  /// the topological order.
  void recomputeFanoutDepths(size_t rootEntryID,
                             size_t oldRootNextEntryID);

  /// Assigns the new links to the given cell.
  /// The number of new links must be the same as the number of old links.
  void relinkCell(size_t entryID, const LinkList &newLinks);

  /// Deletes the given cell and the cells from the transitive fanin cone
  /// whose reference counts have become zero (recursively).
  void deleteCell(size_t entryID);

  /// Checks if the inputs are located at the beginning of the array.
  bool checkInputsOrder() const;
  /// Checks if the outputs are located at the end of the array.
  bool checkOutputsOrder() const;

  /// Sorts entries in topological order accoring to the prev and next arrays.
  /// Removes the holes.
  void rearrangeEntries(
      std::vector<size_t> &entryMapping,
      const bool deleteBufs);

  /// Clears the replacement context.
  void clearContext();

private:
  using StrashMap = std::unordered_map<StrashKey, size_t>;

  /// Returns {invalidID, false} unless strashing is enabled.
  /// Otherwise, returns {entryID, existed (false) / newly created (true)}.
  std::pair<size_t, bool> strashEntry(CellTypeID typeID, const LinkList &links);

  /// Removes the given entry from the strashing map.
  void destrashEntry(size_t entryID);

public:
  static constexpr size_t invalidID = static_cast<size_t>(-1);

  static constexpr size_t normalOrderID = invalidID - 1;
  static constexpr size_t lowerBoundID  = invalidID - 2;
  static constexpr size_t upperBoundID  = invalidID - 3;

private:
  struct EntryDescriptor final {
    EntryDescriptor():
        prev(normalOrderID),
        next(normalOrderID),
        depth(invalidID),
        weight(0.0),
        data(nullptr),
        session(0) {}

    size_t prev;
    size_t next;
    size_t depth;
    float weight;
    void *data;
    size_t session;
  };

  uint16_t nIn;
  uint16_t nOut;

  std::vector<Subnet::Entry> entries;
  bool isDisassembled{false};

  std::vector<EntryDescriptor> desc;
  std::vector<FanoutsContainer> fanouts{};
  bool fanoutsEnabled{false};

  std::vector<std::pair<size_t, size_t>> depthBounds;
  std::vector<size_t> emptyEntryIDs;

  size_t subnetBegin{invalidID};
  size_t subnetEnd{invalidID};

  StrashMap strash;

  size_t sessionID{0};
  bool isSessionStarted{false};
};

//===----------------------------------------------------------------------===//
// Subnet Object
//===----------------------------------------------------------------------===//

class SubnetObject final {
public:
  SubnetObject() {}
  SubnetObject(const SubnetID subnetID): subnetID(subnetID) {}

  SubnetObject(const SubnetObject &other) = delete;
  SubnetObject &operator=(const SubnetObject &other) = delete;

  SubnetObject(SubnetObject &&other) {
    subnetID = other.subnetID;
    subnetBuilder = other.subnetBuilder;
    other.subnetBuilder = nullptr;
  }

  SubnetObject &operator =(SubnetObject &&other) {
    subnetID = other.subnetID;
    subnetBuilder = other.subnetBuilder;
    other.subnetBuilder = nullptr;
    return *this;
  }

  bool isNull() const {
    return subnetID == OBJ_NULL_ID && !subnetBuilder;
  } 

  bool hasObject() const {
    return subnetID != OBJ_NULL_ID;
  }

  bool hasBuilder() const {
    return subnetBuilder != nullptr;
  }

  SubnetID id() const {
    return subnetID;
  }

  const Subnet &object() const {
    assert(subnetID != OBJ_NULL_ID);
    return Subnet::get(subnetID);
  }

  const SubnetBuilder &builder() const {
    assert(subnetBuilder);
    return *subnetBuilder;
  }

  SubnetBuilder &builder(bool rebuild = false) {
    assert(subnetID == OBJ_NULL_ID || rebuild);

    if (!subnetBuilder) {
      subnetBuilder = new SubnetBuilder();
    }

    subnetID = OBJ_NULL_ID;
    return *subnetBuilder;
  }

  SubnetID make() {
    if (subnetID != OBJ_NULL_ID) {
      return subnetID;
    }

    assert(subnetBuilder);
    subnetID = subnetBuilder->make();

    return subnetID;
  }

  void release() {
    if (subnetID != OBJ_NULL_ID) {
      Subnet::release(subnetID);
    }
    if (subnetBuilder) {
      delete subnetBuilder;
      subnetBuilder = nullptr;
    }
  }

  ~SubnetObject() {
    if (subnetBuilder) {
      delete subnetBuilder;
    }
  }

private:
  SubnetBuilder *subnetBuilder{nullptr};
  SubnetID subnetID{OBJ_NULL_ID};
};

} // namespace eda::gate::model
