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
#include "gate/model/iomapping.h"
#include "gate/model/object.h"
#include "gate/model/storage.h"
#include "gate/model/subnet_base.h"
#include "util/hash.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
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
  static std::pair<EntryID, uint16_t> getLinkIndices(EntryID i, uint16_t j) {
    if (j < Cell::InPlaceLinks) {
      return {i, j};
    }
    const auto k = j - Cell::InPlaceLinks;
    return {i + 1 + (k / Cell::InEntryLinks), k % Cell::InEntryLinks};
  }

  using Link = model::SubnetLink;
  using LinkList = model::SubnetLinkList;
  using Cell = model::SubnetCell;
  using Entry = model::SubnetEntry;

  Subnet &operator=(const Subnet &) = delete;
  Subnet(const Subnet &) = delete;

  /// Checks whether the subnet contains only inputs and outputs.
  bool isTrivial() const { return nCell <= nIn + nOut; }

  /// Returns the overall number of entries including entries w/ links.
  SubnetSz size() const { return nEntry; }

  /// Returns the number of inputs.
  SubnetSz getInNum() const { return nIn; }
  /// Returns the number of outputs.
  SubnetSz getOutNum() const { return nOut; }
  /// Returns the number of cells including inputs and outputs.
  SubnetSz getCellNum() const { return nCell; }
  /// Returns the number of buffers.
  SubnetSz getBufNum() const { return nBuf; }

  /// Returns the i-th input index.
  EntryID getInIdx(const uint32_t i) const { return static_cast<EntryID>(i); }
  /// Returns the i-th output index.
  EntryID getOutIdx(const uint32_t i) const {
    return nEntry - nOut + static_cast<EntryID>(i);
  }
  /// Returns the maximum entry index.
  EntryID getMaxIdx() const { return nEntry - 1; }

  /// Returns the i-th cell.
  const Cell &getCell(EntryID i) const { return entries[i].cell; }

  /// Returns the j-th link of the i-th cell.
  const Link &getLink(EntryID i, uint16_t j) const;
  /// Returns the links of the i-th cell.
  const LinkList getLinks(EntryID i) const; // FIXME: Deprecated.
  /// Returns an array filled by the links.
  const Link *getLinks(EntryID i, Link *links, uint16_t &nLinks) const;

  /// Returns the i-th input link.
  Link getIn(EntryID i) const {
    assert(i < nIn);
    return Link(i, 0, 0);
  }

  /// Returns the i-th output link.
  Link getOut(EntryID i) const {
    assert(i < nOut);
    return entries[nEntry - nOut + i].cell.link[0];
  }

  /// Returns the array of entries.
  const Array<Entry> &getEntries() const { return entries; }

  /// Returns the minimum and maximum path lengths.
  std::pair<SubnetSz, SubnetSz> getPathLength() const;

  /// Check if the subnet is tech-mapped.
  bool isTechMapped() const {
    if (isTrivial()) {
      return false;
    }

    for (auto i = nIn; i < nEntry - nOut; ++i) {
      const auto &cell = getCell(i);

      if (cell.isZero() || cell.isOne()) {
        continue;
      }

      // It is assumed that either all cells are logical or
      // all cells are technological (one check is enough).
      return !cell.getType().isGate();
    }

    return false;
  }

private:
  /// Constructs a subnet.
  Subnet(SubnetSz nIn,
         SubnetSz nOut,
         SubnetSz nCell,
         SubnetSz nBuf,
         const std::vector<Entry> &entries):
      nIn(nIn), nOut(nOut), nCell(nCell), nBuf(nBuf), nEntry(entries.size()),
      entries(ArrayBlock<Entry>::allocate(entries, true, true)) {}

  /// Number of inputs.
  const SubnetSz nIn;
  /// Number of outputs.
  const SubnetSz nOut;
  /// Number of cells.
  const SubnetSz nCell;
  /// Number of buffers.
  const SubnetSz nBuf;
  /// Total number of entries.
  const SubnetSz nEntry;

  /// Topologically sorted array of entries.
  const Array<Entry> entries;

  uint64_t padding__{0};
};

static_assert(sizeof(Subnet) == SubnetID::Size);

std::ostream &operator<<(std::ostream &out, const Subnet &subnet);

//===----------------------------------------------------------------------===//
// Subnet Builder
//===----------------------------------------------------------------------===//

class SubnetBuilder;
class SubnetObject;

template <typename T>
using Allocator = std::allocator<T>;

static Allocator<uint64_t> uint64Allocator;

/// SubnetBuilder entries bidirectional iterator.
class EntryIterator {
  friend class SubnetBuilder;

public:
  typedef EntryID value_type;
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

  /// Skips the link entries if required.
  void nextCell();

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
        && (cellLinks.size() <= Cell::InPlaceLinks);
  }

  static bool isEnabled(const Cell &cell) {
    return !cell.isIn()
        && !cell.isOut()
        && cell.arity <= Cell::InPlaceLinks;
  }

  StrashKey(): StrashKey(0, LinkList{}) {}

  StrashKey(const Cell &cell): StrashKey(cell.getTypeID(), cell.getInPlaceLinks()) {}

  StrashKey(CellTypeID cellTypeID, const LinkList &cellLinks):
    typeID(cellTypeID.getSID()), arity(cellLinks.size()) {
    assert(isEnabled(cellTypeID, cellLinks));

    for (uint16_t i = 0; i < cellLinks.size(); ++i) {
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
  Link links[Cell::InPlaceLinks];
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

    for (uint16_t i = 0; i < key.arity; ++i) {
      eda::util::hash_combine<>(h, key.links[i]);
    }

    eda::util::hash_combine<>(h, key.arity);
    eda::util::hash_combine<>(h, key.typeID);

    return h;
  }
};

namespace eda::gate::model {

class SubnetView;

class SubnetBuilder final {
  friend EntryIterator;
  friend SubnetObject;

public:
  using Cell = Subnet::Cell;
  using Entry = Subnet::Entry;
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;

  /// Returns the weight of the cell identified by the index.
  using CellWeightProvider = std::function<float(EntryID)>;
  /// Calculates the real weight used for replace estimation.
  using CellWeightModifier = std::function<float(float, uint16_t)>;
  /// Performs a certain action in a certain situation.
  using CellActionCallback = std::function<void(EntryID)>;
  /// Checks if a cell type satisfies some condition.
  using CellTypePredicate = std::function<bool(CellTypeID)>;

  /// Fanouts container wrapper;
  using FanoutsContainer = std::vector<EntryID>;
  using EntryToEntry = std::unordered_map<EntryID, EntryID>;

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

  static SubnetID makeZero(const SubnetSz nIn);
  static SubnetID makeOne(const SubnetSz nIn);
  static SubnetID makeConst(const SubnetSz nIn, const bool value);

  //FIXME: Make constructors private.
  SubnetBuilder() {
    constexpr auto n = 1024; // FIXME
    entries.reserve(n);
    desc.reserve(n);
    depthBounds.reserve(n);
    strash.reserve(n);
  }

  explicit SubnetBuilder(
      const Subnet &subnet,
      const CellWeightProvider *weightProvider = nullptr):
      SubnetBuilder() {
    const auto inputs = addInputs(subnet.getInNum());
    const auto outputs = addSubnet(subnet, inputs, weightProvider);
    addOutputs(outputs);
  }

  explicit SubnetBuilder(
      SubnetID subnetID,
      const CellWeightProvider *weightProvider = nullptr):
      SubnetBuilder(Subnet::get(subnetID), weightProvider) {}

  SubnetBuilder &operator=(const SubnetBuilder &) = delete;

  /// Checks whether the subnet contains only inputs and outputs.
  bool isTrivial() const { return nCell <= nIn + nOut; }

  /// Returns the number of inputs.
  SubnetSz getInNum() const { return nIn; }
  /// Returns the number of outputs.
  SubnetSz getOutNum() const { return nOut; }
  /// Returns the number of cells including inputs and outputs.
  SubnetSz getCellNum() const { return nCell; }
  /// Returns the number of buffers.
  SubnetSz getBufNum() const { return nBuf; }

  /// Returns the maximum entry index.
  EntryID getMaxIdx() const { return entries.size() - 1; }

  /// Returns the constant reference to the i-th entry.
  const Entry &getEntry(EntryID i) const {
    return entries[i];
  }

  /// Returns the non-constant reference to the i-th entry.
  Entry &getEntry(EntryID i) {
    return entries[i];
  }

  /// Returns the constant reference to the i-th cell.
  const Cell &getCell(EntryID i) const {
    return entries[i].cell;
  }

  /// Returns the non-constant reference to the i-th cell.
  Cell &getCell(EntryID i) {
    return entries[i].cell;
  }

  /// Returns the depth of the i-th cell.
  SubnetDepth getDepth(EntryID i) const {
    return desc[i].depth;
  }

  /// Returns the first cell in the topological order with the passed depth.
  EntryID getFirstWithDepth(SubnetDepth d) const {
    return depthBounds[d].first;
  }

  /// Returns the last cell in the topological order with the passed depth.
  EntryID getLastWithDepth(SubnetDepth d) const {
    return depthBounds[d].second;
  }

  /// Returns the weigth of the i-th cell.
  float getWeight(EntryID i) const {
    return desc[i].weight;
  }

  /// Sets the weigth of the i-th cell.
  void setWeight(EntryID i, float weight) {
    desc[i].weight = weight;
  }

  /// Returns the pointer to the data associated w/ the i-th cell.
  template <typename T>
  const T *getDataPtr(EntryID i) const {
    return static_cast<T*>(desc[i].data);
  }

  /// Sets the pointer to the data associated w/ the i-th cell.
  void setDataPtr(EntryID i, const void *data) {
    desc[i].data = const_cast<void*>(data);
  }

  /// Returns the pointer to the data associated w/ the i-th cell.
  template <typename T>
  const T &getDataVal(EntryID i) const {
    static_assert(sizeof(T) <= sizeof(void*));
    return reinterpret_cast<const T&>(desc[i].data);
  }

  /// Sets the pointer to the data associated w/ the i-th cell.
  template <typename T>
  void setDataVal(EntryID i, const T &data) {
    static_assert(sizeof(T) <= sizeof(void*));
    desc[i].data = reinterpret_cast<void*>(data);
  }

  /// Returns fanouts of the i-th cell.
  FanoutsContainer getFanouts(EntryID i) const {
    assert(fanoutsEnabled);
    assert(i < entries.size());
    if (fanouts.size() <= i) {
      return {};
    }
    return fanouts[i];
  }

  /// Returns the entry/link indices of the j-th link of the i-th entry.
  std::pair<EntryID, uint16_t> getLinkIndices(EntryID i, uint16_t j) const {
    if (j < Cell::InPlaceLinks) {
      return {i, j};
    }

    auto k = j - Cell::InPlaceLinks;
    auto n = getNext(i);

    while (k > Cell::InEntryLinks) {
      k -= Cell::InEntryLinks;
      n = getNext(n);
    }

    return {n, k};
  }

  /// Returns the j-th link of the i-th cell.
  const Link &getLink(EntryID i, uint16_t j) const;
  /// Returns the links of the i-th cell.
  const LinkList getLinks(EntryID i) const; // FIXME: Deprecated.
  /// Returns an array filled by the links.
  const Link *getLinks(EntryID i, Link *links, uint16_t &nLinks) const;

  /// Checks if the subnet is tech-mapped.
  bool isTechMapped() const {
    if (nCell <= (nIn + nOut)) {
      return false;
    }

    // Find the first non-input cell.
    for (auto it = begin(); it != end(); ++it) {
      const auto entryID = *it;
      const auto &cell = getCell(entryID);

      if (cell.isIn() || cell.isZero() || cell.isOne()) {
        continue;
      }

      // It is assumed that either all cells are logical or
      // all cells are technological (one check is enough).
      return !cell.getType().isGate();
    }

    return false;
  }

  /// Adds an input.
  Link addInput() {
    return addCell(IN);
  }

  /// Adds an output.
  Link addOutput(Link link) {
    return addCell(OUT, link);
  }

  /// Adds a multi-output general-type cell.
  LinkList addMultiOutputCell(CellTypeID typeID, const LinkList &links);

  /// Adds a multi-output general-type cell and performs inlining.
  LinkList addCellRecursively(CellTypeID typeID,
                              const LinkList &links,
                              const CellTypePredicate inlinePredicate);

  /// Adds a single-output general-type cell.
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
  LinkList addInputs(SubnetSz nIn) {
    LinkList result(nIn);
    for (SubnetSz i = 0; i < nIn; ++i) {
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
  LinkList addSubnet(const Subnet &subnet,
                     const LinkList &links,
                     const CellWeightProvider *weightProvider = nullptr,
                     const CellTypePredicate *inlinePredicate = nullptr);

  LinkList addSubnet(SubnetID subnetID,
                     const LinkList &links,
                     const CellWeightProvider *weightProvider = nullptr,
                     const CellTypePredicate *inlinePredicate = nullptr) {
    return addSubnet(
        Subnet::get(subnetID), links, weightProvider, inlinePredicate);
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
  void mark(EntryID i) {
    assert(isSessionStarted);
    desc[i].session = sessionID;
  }

  /// Checks whether the given entry is marked (in the current session).
  bool isMarked(EntryID i) const {
    return desc[i].session == sessionID;
  }

  /// Returns the current or the latest session ID.
  uint32_t getSessionID() const {
    return sessionID;
  }

  /// Returns the latest session ID in which the given entry was marked.
  /// If the entry has not beent marked during, returns 0.
  uint32_t getSessionID(EntryID i) const {
    return desc[i].session;
  }

  /// Sets next EntryID attribute for the i-th entry.
  /// Required for entries with the same simulation bits.
  void setNextWithSim(EntryID i, EntryID next) {
    assert(desc[i].simBits && (next == invalidID || desc[next].simBits));
    desc[i].simNext = next;
  }

  /// Sets the simulation bits for the outI-th fanout link of the i-th entry.
  void setSim(EntryID i, uint16_t outI, uint64_t signature) {
    if (!desc[i].simBits) {
      desc[i].simN = getCell(i).getType().getOutNum();
      desc[i].simBits = uint64Allocator.allocate(desc[i].simN);
      for (uint16_t curOutI = 0; curOutI < desc[i].simN; ++curOutI) {
        desc[i].simBits[curOutI] = 0;
      }
    }
    desc[i].simBits[outI] = signature;
  }

  /// Returns the next EntryID attribute for the i-th entry.
  /// Required for entries with the same simulation bits.
  EntryID getNextWithSim(EntryID i) const {
    return desc[i].simNext;
  }

  /// Returns simulation bits for the outI-th fanout link of the i-th entry.
  uint64_t getSim(EntryID i, uint16_t outI) const {
    const auto &cell = getCell(i);
    assert(outI < cell.getType().getOutNum());
    if (!desc[i].simBits) {
      return 0u;
    }
    return desc[i].simBits[outI];
  }

  /// Replaces the given single-output fragment w/ the given subnet (rhs).
  /// rhsToLhs maps the rhs inputs and output to the subnet boundary cells.
  /// Precondition: cell arities <= Cell::InPlaceLinks.
  void replace(
      const SubnetObject &rhs,
      const InOutMapping &iomapping,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr,
      const CellActionCallback *onRecomputedDepth = nullptr);

  /// Replaces the given single-output fragment w/ the given subnet (rhs).
  /// rhsToLhs maps the rhs inputs and output to the subnet boundary cells.
  /// Preconditions:
  /// 1. Cell arities <= Cell::InPlaceLinks.
  /// 2. SubnetView inner mapping maps its parent PIs and PO.
  /// 3. SubnetView is fanout free.
  void replace(
      const SubnetView &rhs,
      const InOutMapping &iomapping,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr,
      const CellActionCallback *onRecomputedDepth = nullptr);

  /// Replaces the given single-output fragment w/ the given subnet (rhs).
  /// rhsToLhs maps the rhs inputs and output to the subnet boundary cells.
  /// Precondition: cell arities <= Cell::InPlaceLinks.
  void replace(
      const SubnetID rhsID,
      const InOutMapping &iomapping,
      const CellWeightProvider *weightProvider = nullptr,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr,
      const CellActionCallback *onRecomputedDepth = nullptr);

  /// Replaces the given single-output fragment w/ the given SubnetBuilder (rhs).
  /// rhsToLhs maps the rhs inputs and output to the subnet boundary cells.
  /// Precondition: cell arities <= Cell::InPlaceLinks.
  void replace(
      const SubnetBuilder &rhsBuilder,
      const InOutMapping &iomapping,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr,
      const CellActionCallback *onRecomputedDepth = nullptr);

  /// Returns the effect of the replacement with rhs.
  Effect evaluateReplace(
      const SubnetObject &rhs,
      const InOutMapping &iomapping,
      const CellWeightModifier *weightModifier = nullptr) const;

  /// Returns the effect of the replacement with rhs.
  Effect evaluateReplace(
      const SubnetView &rhs,
      const InOutMapping &iomapping,
      const CellWeightModifier *weightModifier = nullptr) const;

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
  /// The number of links in both cells must be <= Cell::InPlaceLinks.
  Link replaceCell(
      EntryID entryID, CellTypeID typeID, const LinkList &links,
      bool delZeroRefcount = true,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onRecomputedDepth = nullptr);

  /// Merges the cells from each map item leaving the one stored in the key.
  /// Precondition: remaining entries precede the entries being removed.
  using EntrySet = std::unordered_set<EntryID>;
  using MergeMap = std::unordered_map<EntryID, EntrySet>;
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
      std::vector<EntryID> &entryMapping,
      const bool deleteBufs = false) {
    assert(/* Constant nets have no inputs */ nOut > 0 && !entries.empty());

    if (isDisassembled || deleteBufs) {
      rearrangeEntries(entryMapping, deleteBufs);
    }
    assert(checkInputsOrder() && checkOutputsOrder());

    return allocateObject<Subnet>(nIn, nOut, nCell, nBuf, std::move(entries));
  }

  /// @brief Makes a subnet.
  /// @param deleteBufs If set all BUF cells will be deleted.
  SubnetID make(const bool deleteBufs = false) {
    std::vector<EntryID> mapping{};
    return make(mapping, deleteBufs);
  }

private:
  /// Template replace method.
  template <typename RhsContainer, typename RhsIterable, typename RhsIt>
  void replace(
      const RhsContainer &rhsContainer,
      const RhsIterable &rhsIterable,
      const EntryID rhsOutEntryID,
      const InOutMapping &iomapping,
      EntryToEntry &rhsToLhs,
      const std::function<EntryID(RhsIt iter, EntryID i)> &getEntryID,
      const CellWeightProvider *weightProvider = nullptr,
      const CellActionCallback *onNewCell = nullptr,
      const CellActionCallback *onEqualDepth = nullptr,
      const CellActionCallback *onGreaterDepth = nullptr,
      const CellActionCallback *onRecomputedDepth = nullptr);

  /// Template replace evaluating method.
  template <typename RhsContainer>
  Effect evaluateReplace(
      const RhsContainer &rhsContainer,
      const InOutMapping &iomapping,
      const CellWeightProvider *weightProvider = nullptr,
      const CellWeightModifier *weightModifier = nullptr) const;

  /// Increments virtual refcount of reused rhsEntryID links.
  /// Returns weight delta after incrementing links refcount.
  template <typename RhsContainer>
  float incOldLinksRefcnt(
      const RhsContainer &rhsContainer,
      const EntryID rhsEntryID,
      const EntryToEntry &rhsToLhs,
      std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
      const CellWeightModifier *weightModifier) const;

  /// Template new entries evaluating method.
  template <typename RhsContainer, typename RhsIterable, typename RhsIt>
  Effect newEntriesEval(
      const RhsContainer &rhsContainer,
      const RhsIterable &rhsIterable,
      const InOutMapping &iomapping,
      EntryToEntry &rhsToLhs,
      const std::function<EntryID(RhsIt iter, EntryID i)> &getEntryID,
      std::unordered_set<EntryID> &reusedLhsEntries,
      std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
      const CellWeightProvider *weightProvider,
      const CellWeightModifier *weightModifier) const;

  /// Returns the add-effect of the replacement:
  /// the number of cells (value of weight) added and new depth of the root.
  Effect newEntriesEval(
      const SubnetView &rhs,
      const InOutMapping &iomapping,
      std::unordered_set<EntryID> &reusedLhsEntries,
      std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
      const CellWeightProvider *weightProvider,
      const CellWeightModifier *weightModifier) const;

  /// Returns the add-effect of the replacement:
  /// the number of cells (value of weight) added and new depth of the root.
  Effect newEntriesEval(
      const Subnet &rhs,
      const InOutMapping &iomapping,
      std::unordered_set<EntryID> &reusedLhsEntries,
      std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
      const CellWeightProvider *weightProvider,
      const CellWeightModifier *weightModifier) const;

  /// Returns the add-effect of the replacement:
  /// the number of cells (value of weight) added and new depth of the root.
  Effect newEntriesEval(
      const SubnetBuilder &rhsBuilder,
      const InOutMapping &iomapping,
      std::unordered_set<EntryID> &reusedLhsEntries,
      std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
      const CellWeightProvider *weightProvider,
      const CellWeightModifier *weightModifier) const;

  /// Returns the delete-effect of the replacement:
  /// the number of cells (value of weight) deleted.
  Effect deletedEntriesEval(
      const EntryID lhsRootEntryID,
      std::unordered_set<EntryID> &reusedLhsEntries,
      std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
      const CellWeightModifier *weightModifier) const;

  /// Return the reference to the j-th link of the given cell.
  Link &getLinkRef(EntryID entryID, uint16_t j);

  /// Updates deleted entry depth bounds and deletes it from the topological
  /// order.
  void deleteDepthBounds(EntryID entryID);

  /// Updates added entry depth bounds and adds it to the topological order.
  void addDepthBounds(EntryID entryID);

  /// Adds fanoutID index in the sourceID fanouts storage
  /// (if fanouts storing is enabled).
  void addFanout(EntryID sourceID, EntryID fanoutID);

  /// Deletes fanoutID index from the sourceID fanouts storage
  /// (if fanouts storing is enabled).
  void delFanout(EntryID sourceID, EntryID fanoutID);

  /// Allocates an entry and returns its index.
  EntryID allocEntry(bool isBuf);
  /// Returns an entry of the given type or allocates a new one.
  EntryID allocEntry(CellTypeID typeID, const LinkList &links);

  /// Deallocates the given entry.
  void deallocEntry(EntryID entryID);

  /// Returns first entry index in topological order.
  EntryID getSubnetBegin() const {
    return subnetBegin == normalOrderID ? 0 : subnetBegin;
  }

  /// Returns last entry index in topological order.
  EntryID getSubnetEnd() const {
    return subnetEnd == normalOrderID ? entries.size() - 1 : subnetEnd;
  }

  /// Returns the index of the next entry (topological ordering).
  EntryID getNext(EntryID entryID) const;

  /// Returns the index of the previous entry (topological ordering).
  EntryID getPrev(EntryID entryID) const;

  /// Specifies the order between the given entries.
  void setOrder(EntryID firstID, EntryID secondID);

  /// Places the given entry after the pivot.
  /// The given entry should not be in the topological order.
  void placeAfter(EntryID entryID, EntryID pivotEntryID);

  /// Places the given entry before the pivot.
  /// The given entry should not be in the topological order.
  void placeBefore(EntryID entryID, EntryID pivotEntryID);

  /// Recursively recomputes the root fanouts depths and updates positions in
  /// the topological order.
  void recomputeFanoutDepths(
      EntryID rootEntryID,
      EntryID oldRootNextEntryID,
      const CellActionCallback *onRecomputedDepth = nullptr);

  /// Assigns the new links to the given cell.
  /// The number of new links must be the same as the number of old links.
  void relinkCell(EntryID entryID, const LinkList &newLinks);

  /// Deletes the given cell and the cells from the transitive fanin cone
  /// whose reference counts have become zero (recursively).
  void deleteCell(EntryID entryID);

  /// Checks if the inputs are located at the beginning of the array.
  bool checkInputsOrder() const;
  /// Checks if the outputs are located at the end of the array.
  bool checkOutputsOrder() const;

  /// Sorts entries in topological order accoring to the prev and next arrays.
  /// Removes the holes.
  void rearrangeEntries(
      std::vector<EntryID> &entryMapping,
      const bool deleteBufs);

  /// Clears the replacement context.
  void clearContext();

private:
  using StrashMap = std::unordered_map<StrashKey, EntryID>;

  /// Returns {invalidID, false} unless strashing is enabled.
  /// Otherwise, returns {entryID, existed (false) / newly created (true)}.
  std::pair<EntryID, bool> strashEntry(CellTypeID typeID, const LinkList &links);

  /// Removes the given entry from the strashing map.
  void destrashEntry(EntryID entryID);

public:
  static constexpr SubnetDepth invalidDepth = static_cast<SubnetDepth>(-1);

  static constexpr EntryID invalidID = static_cast<EntryID>(-1);

  static constexpr EntryID normalOrderID = invalidID - 1;
  static constexpr EntryID lowerBoundID  = invalidID - 2;
  static constexpr EntryID upperBoundID  = invalidID - 3;

private:
  struct EntryDescriptor final {
    EntryDescriptor():
        prev(normalOrderID),
        next(normalOrderID),
        depth(invalidDepth),
        weight(0.),
        data(nullptr),
        session(0),
        simNext(invalidID),
        simBits(nullptr),
        simN(0) {}

    EntryDescriptor(const EntryDescriptor &other) : EntryDescriptor() {
      this->prev = other.prev;
      this->next = other.next;
      this->depth = other.depth;
      this->weight = other.weight;
    }

    EntryDescriptor(EntryDescriptor &&other) : EntryDescriptor() {
      this->prev = std::move(other.prev);
      this->next = std::move(other.prev);
      this->depth = std::move(other.prev);
      this->weight = std::move(other.prev);
    }

    EntryDescriptor &operator=(const EntryDescriptor &other) {
      this->prev = other.prev;
      this->next = other.next;
      this->depth = other.depth;
      this->weight = other.weight;
      this->data = nullptr;
      this->session = 0;
      if (simBits) {
        uint64Allocator.deallocate(simBits, simN);
        simBits = nullptr;
        this->simNext = invalidID;
        this->simN = 0;
      }
      return *this;
    }

    EntryDescriptor &operator=(EntryDescriptor &&other) {
      this->prev = std::move(other.prev);
      this->next = std::move(other.next);
      this->depth = std::move(other.depth);
      this->weight = std::move(other.weight);
      this->data = nullptr;
      this->session = 0;
      if (simBits) {
        uint64Allocator.deallocate(simBits, simN);
        simBits = nullptr;
        this->simNext = invalidID;
        this->simN = 0;
      }
      return *this;
    }

    ~EntryDescriptor() {
      if (simBits) {
        uint64Allocator.deallocate(simBits, simN);
        simBits = nullptr;
      }
    }

    EntryID prev;
    EntryID next;
    SubnetDepth depth;
    float weight;
    void *data;
    uint32_t session;

    EntryID simNext;
    uint64_t *simBits;
    uint16_t simN;
  };

  SubnetSz nIn{0};
  SubnetSz nOut{0};
  SubnetSz nCell{0};
  SubnetSz nBuf{0};

  std::vector<Entry> entries;
  bool isDisassembled{false};

  std::vector<EntryDescriptor> desc;
  std::vector<FanoutsContainer> fanouts{};
  bool fanoutsEnabled{false};

  std::vector<std::pair<EntryID, EntryID>> depthBounds;
  std::vector<EntryID> emptyEntryIDs;

  EntryID subnetBegin{invalidID};
  EntryID subnetEnd{invalidID};

  StrashMap strash;

  uint32_t sessionID{0};
  bool isSessionStarted{false};
};

//===----------------------------------------------------------------------===//
// Subnet Object
//===----------------------------------------------------------------------===//

class SubnetObject final {
public:
  SubnetObject() {}
  SubnetObject(const SubnetID subnetID): subnetID(subnetID) {}
  SubnetObject(const std::shared_ptr<SubnetBuilder> &builderPtr):
      subnetBuilderPtr(builderPtr) {}

  SubnetObject(const SubnetObject &) = delete;
  SubnetObject &operator=(const SubnetObject &) = delete;

  SubnetObject(SubnetObject &&other) {
    subnetID = other.subnetID;
    subnetBuilderPtr = other.subnetBuilderPtr;
    other.subnetBuilderPtr.reset();
  }

  SubnetObject &operator=(SubnetObject &&other) {
    subnetID = other.subnetID;
    subnetBuilderPtr = other.subnetBuilderPtr;
    other.subnetBuilderPtr.reset();
    return *this;
  }

  bool isNull() const {
    return subnetID == OBJ_NULL_ID && !subnetBuilderPtr;
  }

  bool hasObject() const {
    return subnetID != OBJ_NULL_ID;
  }

  bool hasBuilder() const {
    return subnetBuilderPtr != nullptr;
  }

  SubnetID id() const {
    assert(subnetID != OBJ_NULL_ID);
    return subnetID;
  }

  const Subnet &object() const {
    assert(subnetID != OBJ_NULL_ID);
    return Subnet::get(subnetID);
  }

  const SubnetBuilder &builder() const {
    if (!subnetBuilderPtr) {
      subnetBuilderPtr = (subnetID != OBJ_NULL_ID)
          ? std::make_shared<SubnetBuilder>(subnetID)
          : std::make_shared<SubnetBuilder>();
    }

    return *subnetBuilderPtr;
  }

  SubnetBuilder &builder() {
    if (!subnetBuilderPtr) {
      subnetBuilderPtr = (subnetID != OBJ_NULL_ID)
          ? std::make_shared<SubnetBuilder>(subnetID)
          : std::make_shared<SubnetBuilder>();
    }

    return *subnetBuilderPtr;
  }

  const std::shared_ptr<SubnetBuilder> &builderPtr() const {
    return subnetBuilderPtr;
  }

  SubnetID make() const {
    if (subnetID != OBJ_NULL_ID) {
      return subnetID;
    }

    assert(subnetBuilderPtr);
    subnetID = subnetBuilderPtr->make();

    return subnetID;
  }

  const Subnet &makeObject() {
    return Subnet::get(make());
  }

  void release() {
    if (subnetID != OBJ_NULL_ID) {
      Subnet::release(subnetID);
    }
    if (subnetBuilderPtr) {
      subnetBuilderPtr.reset();
    }
  }

  ~SubnetObject() = default;

private:
  mutable std::shared_ptr<SubnetBuilder> subnetBuilderPtr{nullptr};
  mutable SubnetID subnetID{OBJ_NULL_ID};
};

} // namespace eda::gate::model
