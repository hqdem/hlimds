//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/logger.h"
#include "gate/model/decomposer/net_decomposer.h"

#include <cassert>
#include <cstdint>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Common
//===----------------------------------------------------------------------===//

using LinkVec = std::vector<Link>;
using LinkSet = std::unordered_set<Link>;
using LinkMap = NetDecomposer::LinkMap;
using SignalType = NetDecomposer::SignalType;
using LinkDescVec = NetDecomposer::LinkDescVec;
using LinkToDesc = NetDecomposer::LinkToDesc;
using ConnectionDesc = NetDecomposer::ConnectionDesc;

using CellVec = std::vector<CellID>;
using CellSet = std::unordered_set<CellID>;
using CellMap = NetDecomposer::CellMap;

using CellMapping = NetDecomposer::CellMapping;
using EntryToDesc = NetDecomposer::EntryToDesc;

/// Aggregates cell information.
struct CellInfo final {
  const CellID cellID;
  const Cell &cell;
  const CellType &type;
};

/// Gets information on the given cell.
static inline CellInfo getCellInfo(const CellID cellID) {
  const auto &cell = Cell::get(cellID);
  const auto &type = cell.getType();

  return CellInfo{cellID, cell, type};
}

/// Gets information on the given cell (link-end).
static inline CellInfo getCellInfo(const LinkEnd linkEnd) {
  return getCellInfo(linkEnd.getCellID());
}

/// Checks if the provided type has DFF pinout.
static inline bool dffPinout(const CellType &type) {
  return type.isDff() || type.isSDff() || type.isADff() || type.isDffRs();
}

/// Checks if the provided type has DLATCH pinout.
static inline bool dlatchPinout(const CellType &type) {
  return type.isDLatch() || type.isADLatch() || type.isDLatchRs();
}

/// Checks if the provided type has LATCHrs pinout.
static inline bool latchRsPinout(const CellType &type) {
  return type.isLatchRs();
}

static inline SignalType getSignalType(const Cell &cell, const uint16_t j) {
  const auto &cellType = cell.getType();
  if (cellType.isIn() || cellType.isOut()) {
    return SignalType::DATA;
  }
  if (dffPinout(cellType)) {
    switch (j) {
      case CellPin::DFF_IN_D: return SignalType::DATA;
      case CellPin::DFF_IN_CLK: return SignalType::CLOCK;
      case CellPin::DFF_IN_RST: return SignalType::RESET;
      case CellPin::DFF_IN_SET: return SignalType::SET;
      default: assert(false && "Link number is too high for the cell type");
    }
  }
  if (dlatchPinout(cellType)) {
    switch (j) {
      case CellPin::DLATCH_IN_D: return SignalType::DATA;
      case CellPin::DLATCH_IN_ENA: return SignalType::ENABLE;
      case CellPin::DLATCH_IN_RST: return SignalType::RESET;
      case CellPin::DLATCH_IN_SET: return SignalType::SET;
      default: assert(false && "Link number is too high for the cell type");
    }
  }
  if (latchRsPinout(cellType)) {
    switch (j) {
      case CellPin::LATCHrs_IN_RST: return SignalType::RESET;
      case CellPin::LATCHrs_IN_SET: return SignalType::SET;
      default: assert(false && "Link number is too high for the cell type");
    }
  }
  return SignalType::DATA;
}

//===----------------------------------------------------------------------===//
// Decompositor
//===----------------------------------------------------------------------===//

/// Prepares the link to be an input mapping key.
static inline Link makeInputLink(const Link &link) {
  return Link{link.source.getCellID(), link.source.getPort(), OBJ_NULL_ID, 0};
}

/// Prepares the link to be an output mapping key.
static inline Link makeOutputLink(const Link &link) {
  return link;
}

/// Makes a subnet link.
static inline Subnet::Link makeLink(
    const size_t index, const uint16_t port, const bool inv) {
  const auto idx = static_cast<uint32_t>(index);
  const auto out = static_cast<uint8_t>(port);
  return Subnet::Link{idx, out, inv};
}

/// Makes a subnet link for the given net link-end.
static inline Subnet::Link makeLink(
    const LinkEnd source, const CellMapping &mapping) {
  const auto i = mapping.inners.find(source.getCellID());
  if (i != mapping.inners.end()) {
    const auto &links = i->second;
    return links[source.getPort()];
  }

  const Link inputLink{source.getCellID(), source.getPort(), OBJ_NULL_ID, 0};
  const auto j = mapping.inputs.find(inputLink);
  assert(j != mapping.inputs.end());

  return makeLink(j->second, 0, false);
}

/// Makes a subnet link list for the given net cell.
static inline Subnet::LinkList makeLinkList(
    const Cell &cell, const CellMapping &mapping) {
  Subnet::LinkList links;
  for (const auto &link : cell.getLinks()) {
    links.push_back(makeLink(link, mapping));
  }
  return links;
}

/// Checks if the link is an input (a primary input or a block output).
static inline bool isInputLink(const Link &link) {
  const auto info = getCellInfo(link.source);
  return info.type.isIn()
      || info.type.isSeqGate()
      || info.type.isHard()
      || (info.type.isSoft() && !info.type.isSubnet());
}

/**
 * @brief Stores the cell links to the given list and specifies connnected links
 * descriptors.
 */
static inline void fillLinks(
    const CellID cellID,
    const ConnectionDesc *descToPropagate,
    LinkDescVec &result) {
  const auto &cell = Cell::get(cellID);
  const auto links = cell.getLinks();
  for (uint16_t j = 0; j != links.size(); ++j) {
    SignalType linkSignalType = (descToPropagate != nullptr) ?
        descToPropagate->signalType : SignalType::DATA;
    if (cell.getType().isSeqGate() && descToPropagate == nullptr) {
      linkSignalType = getSignalType(cell, j);
    }
    const auto link = Link{links[j], LinkEnd{cellID, j}};
    result.push_back({link, ConnectionDesc{linkSignalType}});
  }
}

/**
 * @brief Stores the cells links to the given list and specifies connected links
 * descriptors.
 */
static inline void fillLinks(
    const List<CellID> &cells,
    const ConnectionDesc *descToPropagate,
    LinkDescVec &result) {
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    const auto &type = Cell::get(*i).getType();
    if (!(type.isSoft() && type.isSubnet())) {
      fillLinks(*i, descToPropagate, result);
    }
  }
}

/// Returns the cell links with connection descriptors.
static inline LinkDescVec getLinks(
    const CellID cellID,
    const ConnectionDesc *desc) {
  LinkDescVec links{};
  fillLinks(cellID, desc, links);
  return links;
}

/// Returns the component outputs (primary outputs and block inputs).
static inline LinkDescVec extractOutputs(const Net &net) {
  LinkDescVec result{};
  result.reserve(net.getCellNum());

  fillLinks(net.getOutputs(),    nullptr, result);
  fillLinks(net.getFlipFlops(),  nullptr, result);
  fillLinks(net.getSoftBlocks(), nullptr, result); // Skip synthesizable blocks.
  fillLinks(net.getHardBlocks(), nullptr, result);

  return result;
}

/// Describes a subnet.
struct NetComponent final {
  /// Inputs are links of the form <(src-cell:src-port), (0:0)>,
  /// i.e. only sources matter.
  LinkSet inputs;
  /// Connnection descriptors of the primary inputs of the component.
  LinkToDesc inputsDesc;
  /// Inner cells are just cells, not links (topologically sorted).
  CellVec inners;
  /// Outputs are links of the form <(src-cell:src-port), (dst-cell:dst-port)>,
  /// i.e. targets matter (especially flip-flops).
  LinkSet outputs;
  /// Connnection descriptors of the primary ouputs of the component.
  LinkToDesc outputsDesc;

  /// Checks whether the component is empty.
  bool empty() const {
    return outputs.empty();
  }

  /// Resets the component state.
  void clear() {
    inputs.clear();
    inputsDesc.clear();
    inners.clear();
    outputs.clear();
    outputsDesc.clear();
  }

  /// Merges the component w/ this one.
  void merge(const NetComponent &rhs) {
    inputs.insert(rhs.inputs.begin(), rhs.inputs.end());
    inputsDesc.insert(rhs.inputsDesc.begin(), rhs.inputsDesc.end());
    inners.insert(inners.end(), rhs.inners.begin(), rhs.inners.end());
    outputs.insert(rhs.outputs.begin(), rhs.outputs.end());
    outputsDesc.insert(rhs.outputsDesc.begin(), rhs.outputsDesc.end());
  }
};

/// Traveral stack entry.
struct NetTraversalEntry final {
  /// Checks if the entry corresponds to an input.
  bool isInput() const { return isInputLink(getLink()); }
  /// Checks if the entry corresponds to an output.
  bool isOutput() const { return cellID == OBJ_NULL_ID; }
  /// Checks if the entry is fully traversed.
  bool isPassed() const { return linkIndex >= linkDesc.size(); }
  /// Returns the current link of the entry.
  const Link &getLink() const { return linkDesc[linkIndex].first; }
  /// Returns the current link descriptor of the entry.
  const ConnectionDesc &getLinkDesc() const {
    return linkDesc[linkIndex].second;
  }

  const CellID cellID;
  const LinkDescVec linkDesc;

  size_t linkIndex;
};

/// Traversal stack.
using NetTraversalStack =
    std::stack<NetTraversalEntry, std::vector<NetTraversalEntry>>;

/// Traversal context.
struct NetTraversalContext final {
  /// Constructs the initial context.
  NetTraversalContext(const Net &net) {
    belongsTo.reserve(net.getCellNum());

    components.reserve(1024*1024);     // FIXME:
    componentCells.reserve(1024*1024); // FIXME:

    const auto outputs = extractOutputs(net);

    if (!outputs.empty()) {
      stack.push(NetTraversalEntry{OBJ_NULL_ID, outputs, 0});
    } else {
      UTOPIA_LOG_WARN("Net has no outputs");
    }
  }

  /// Checks if the traversal is completed.
  bool isCompleted() const { return stack.empty(); }
  /// Returns the top entry of the stack.
  NetTraversalEntry &top() { return stack.top(); }
  /// Pops the top entry from the stack.
  void pop() { stack.pop(); }

  /// Checks if the cell is new and pushes it to the stack.
  void push(const CellID cellID, const ConnectionDesc *desc) {
    const auto i = belongsTo.find(cellID);
    if (i != belongsTo.end()) {
      merging.insert(i->second);
      return;
    }

    const auto j = componentCells.find(cellID);
    if (j == componentCells.end()) {
      componentCells.insert(cellID);
      stack.push(NetTraversalEntry{cellID, getLinks(cellID, desc), 0});
    }
  }

  /// Adds the input link with its descriptor to the current component.
  void addInput(const Link &link, const ConnectionDesc &linkDesc) {
    // Using a set avoids duplicates.
    component.inputs.insert(link);
    component.inputsDesc[link] = linkDesc;
  }

  /// Adds the inner cell to the current component.
  void addInner(const CellID cellID) {
    component.inners.push_back(cellID);
  }

  /// Adds the output link with its descriptor to the current component.
  void addOutput(const Link &link, const ConnectionDesc &linkDesc) {
    component.outputs.insert(link);
    component.outputsDesc[link] = linkDesc;
  }

  /// Merge the current component w/ the ones it depends on.
  NetComponent &mergeComponents(
      const std::set<size_t> &merging, const size_t index) {
    assert(merging.find(index) != merging.end());

    auto &root = components[index];
    for (auto i = merging.begin(); i != merging.end(); ++i) {
      if (*i != index) {
        auto &next = components[*i];
        root.merge(next);

        for (const auto &cellID : next.inners) {
          belongsTo[cellID] = index;
        }

        next.clear();
      }
    }

    return root;
  }

  /// Adds the previously constructed component to the list.
  void makeComponent() {
    const size_t index = merging.empty() ? components.size() : *merging.begin();

    if (!merging.empty()) {
      mergeComponents(merging, index).merge(component);
    } else {
      components.push_back(component);
    }

    for (const auto &cellID : component.inners) {
      belongsTo[cellID] = index;
    }

    // Reset the component state (start building a new one).
    merging.clear();
    component.clear();
    componentCells.clear();
  }

  /// Increments the link index of the top entry.
  void nextLink() {
    stack.top().linkIndex++;
  }

  /// Stores the constructed components (including empty ones).
  std::vector<NetComponent> components;

  /// Maps cells to components.
  std::unordered_map<CellID, size_t> belongsTo;
  /// Components to be merged w/ the current one.
  std::set<size_t> merging;

  /// Component under construction.
  NetComponent component;
  /// Stores the current component's inner cells.
  CellSet componentCells;

  /// Traversal stack (DFS from outputs to inputs).
  NetTraversalStack stack;
};

/// Returns the net components (connected subnets).
static std::vector<NetComponent> extractComponents(const Net &net) {
  NetTraversalContext context(net);

  // DFS net traversal started from the outputs.
  while (!context.isCompleted()) {
    auto &entry = context.top();

    if (entry.isOutput() && entry.linkIndex > 0) {
      context.makeComponent();

      // Stop traversal if all outputs have been passed.
      if (entry.isPassed()) {
        context.pop();
        break;
      }
    }

    if (!entry.isPassed()) {
      const auto &link = entry.getLink();
      const auto &linkDesc = entry.getLinkDesc();
      const auto isInput = entry.isInput();
      const auto isOutput = entry.isOutput();

      if (isOutput) {
        context.addOutput(makeOutputLink(link), linkDesc);
      }
      if (isInput) {
        context.addInput(makeInputLink(link), linkDesc);
      }

      context.nextLink();

      if (!isInput) {
        context.push(link.source.getCellID(), &linkDesc);
      }
    } else {
      // For topological ordering, add a cell right before popping.
      context.addInner(entry.cellID);
      context.pop();
    }
  }

  return context.components;
}

/**
 * @brief Makes a subnet for the given net component and specifies IO
 *        connection descriptors.
 */
static SubnetID makeSubnet(const Net &net,
                           const NetComponent &component,
                           CellMapping &mapping,
                           EntryToDesc &entryToDesc) {
  SubnetBuilder subnetBuilder;

  for (const auto &input : component.inputs) {
    const auto info = getCellInfo(input.source);
    const auto link = info.type.isCombinational()
        ? subnetBuilder.addInput()
        : subnetBuilder.addInput(info.cellID.getSID());

    const auto inputLink = makeInputLink(input);
    mapping.inputs.emplace(inputLink, link.idx);

    entryToDesc[link.idx] = component.inputsDesc.find(input)->second;
  } // for component inputs.

  for (const auto &inner : component.inners) {
    const auto info = getCellInfo(inner);
    const auto ilinks = makeLinkList(info.cell, mapping);

    Subnet::LinkList olinks;

    if (info.type.isSoft()) {
      // Inline the soft block implementation (subnet).
      assert(info.type.isSubnet());
      olinks = subnetBuilder.addSubnet(info.type.getSubnet(), ilinks);
    } else {
      const auto neg = info.type.isNegative();
      const auto sym = info.type.getSymbol();

      Subnet::Link olink;

      if (sym == BUF || sym == NOT) {
        olink = makeLink(info.cell.getLink(0), mapping);
      } else {
        olink = subnetBuilder.addCell((neg ? getNegSymbol(sym) : sym), ilinks);
      }

      olinks.push_back(neg ? ~olink : olink);
    }

    mapping.inners.emplace(info.cellID, olinks);
  } // for component inner cells.

  assert(!component.outputs.empty());
  for (const auto &output : component.outputs) {
    const auto info = getCellInfo(output.target);
    const auto ilink = makeLink(output.source, mapping);
    const auto olink = info.type.isCombinational()
        ? subnetBuilder.addOutput(ilink)
        : subnetBuilder.addOutput(ilink, info.cellID.getSID());

    const auto outputLink = makeOutputLink(output);
    mapping.outputs.emplace(outputLink, olink.idx);

    entryToDesc[olink.idx] = component.outputsDesc.find(output)->second;
  } // for component outputs.

  const auto subnetID = subnetBuilder.make();
  const auto &subnet = Subnet::get(subnetID);

  // Original subnet size is required for proper composition.
  mapping.size = subnet.size();

  return subnetID;
}

bool NetDecomposer::decompose(const NetID netID, Result &result) const {
  assert(netID != OBJ_NULL_ID);
  const auto &net = Net::get(netID);

  // Store the net interface.
  result.inputs = net.getInputs();
  result.outputs = net.getOutputs();

  const auto components = extractComponents(net);

  assert(result.subnets.empty());
  result.subnets.reserve(components.size());

  for (const auto &component : components) {
    if (component.empty()) {
      continue;
    }
    if (component.inputs.empty()) {
      UTOPIA_LOG_WARN("Non-empty net component has no inputs");
    }

    CellMapping mapping;
    EntryToDesc entryToDesc;

    const auto subnetID = makeSubnet(net, component, mapping, entryToDesc);
    result.subnets.emplace_back(subnetID, mapping, entryToDesc);
  }

  return true;
}

bool NetDecomposer::decompose(const SubnetID subnetID, Result &result) const {
  assert(subnetID != OBJ_NULL_ID);
  const auto &subnet = Subnet::get(subnetID);

  assert(result.subnets.empty());

  CellMapping cellMapping;
  EntryToDesc entryToDesc;

  cellMapping.size = subnet.size();

  for (size_t i = 0; i < subnet.getInNum(); ++i) {
    const auto cellID = makeCell(IN);
    result.inputs.push_back(cellID);

    const Link link{cellID, 0, OBJ_NULL_ID, 0};
    cellMapping.inputs.emplace(link, i);
    entryToDesc[subnet.getInIdx(i)] = {getSignalType(Cell::get(cellID), 0)};
  }

  for (size_t i = 0; i < subnet.getOutNum(); ++i) {
    const auto cellID = makeCell(OUT, LinkEnd());
    result.outputs.push_back(cellID);

    const Link link{OBJ_NULL_ID, 0, cellID, 0};
    cellMapping.outputs.emplace(link, subnet.size() - subnet.getOutNum() + i);
    entryToDesc[subnet.getOutIdx(i)] = {getSignalType(Cell::get(cellID), 0)};
  }

  result.subnets.emplace_back(subnetID, cellMapping, entryToDesc);
  return true;
}

//===----------------------------------------------------------------------===//
// Compositor
//===----------------------------------------------------------------------===//

/// Stores information on a subnet cell.
struct CellDescriptor final {
  /// Cell kind.
  enum { INPUT, OUTPUT, INNER } kind;
  /// Describes an input/output cell.
  Link link;
  /// Describes an inner cell.
  CellID cellID;
};

/// Maps subnet cell indices to cell descriptors.
using InverseCellMapping = std::vector<CellDescriptor>;
/// Maps old cells (inputs/outputs/flip-flops/blocks) to new ones.
using InOutCellMapping = std::unordered_map<CellID, CellID>;

/// Makes a link-end corresponding to the given subnet link.
static inline LinkEnd makeLinkEnd(NetBuilder &netBuilder,
                                  const Subnet::Link &link,
                                  const InverseCellMapping &inverse) {
  const auto &descriptor = inverse[link.idx];

  const LinkEnd source = (descriptor.kind == CellDescriptor::INNER)
      ? LinkEnd{descriptor.cellID, static_cast<uint16_t>(link.out)}
      : descriptor.link.source;

  if (!link.inv) {
    return source;
  }

  const auto cellID = makeCell(NOT, source);
  netBuilder.addCell(cellID);

  return LinkEnd{cellID, 0};
}

/// Makes a link list corresponding to the given subnet cell.
static Cell::LinkList makeLinkList(NetBuilder &netBuilder,
                                   const Subnet::LinkList &links,
                                   const InverseCellMapping &inverse) {
  Cell::LinkList result(links.size());

  for (uint16_t port = 0; port < links.size(); ++port) {
    result[port] = makeLinkEnd(netBuilder, links[port], inverse);
  }

  return result;
}

/// Makes a new boundary cell for the given old one.
static CellID makeCell(NetBuilder &netBuilder,
                       const CellID oldCellID,
                       InOutCellMapping &inout) {
  const auto i = inout.find(oldCellID);

  // Already created.
  if (i != inout.end()) {
    return i->second;
  }

  const auto &oldCell = Cell::get(oldCellID);
  const Cell::LinkList invalidLinks(oldCell.getFanin());
  const auto newCellID = makeCell(oldCell.getTypeID(), invalidLinks);

  netBuilder.addCell(newCellID);
  inout.emplace(oldCellID, newCellID);

  return newCellID;
}

/// Makes a new inner cell for the given subnet cell.
static CellID makeCell(NetBuilder &netBuilder,
                       const Subnet &subnet,
                       const size_t idx,
                       const InverseCellMapping &inverse) {
  const auto &entries = subnet.getEntries();
  const auto &cell = entries[idx].cell;

  const auto links = makeLinkList(netBuilder, subnet.getLinks(idx), inverse);
  const auto newCellID = makeCell(cell.getTypeID(), links);
  netBuilder.addCell(newCellID);

  return newCellID;
}

/// Makes cells for the components' inputs.
static void makeCellsForInputs(NetBuilder &netBuilder,
                               const Subnet &subnet,
                               const CellMapping &mapping,
                               InverseCellMapping &inverse,
                               InOutCellMapping &inout) {
  assert(subnet.getInNum() == mapping.inputs.size());

  for (const auto &[oldLink, oldIdx] : mapping.inputs) {
    // Inputs are located at the beginning.
    const auto newIdx = oldIdx;
    assert(newIdx < subnet.getInNum());

    const auto oldSourceID = oldLink.source.getCellID();
    const auto newSourceID = makeCell(netBuilder, oldSourceID, inout);

    Link newLink{newSourceID, oldLink.source.getPort(), OBJ_NULL_ID, 0};
    inverse[newIdx] =
        CellDescriptor{CellDescriptor::INPUT, newLink, OBJ_NULL_ID};
  }
}

/// Makes cells for the components' inner cells.
static void makeCellsForInners(NetBuilder &netBuilder,
                               const Subnet &subnet,
                               const CellMapping &mapping,
                               InverseCellMapping &inverse) {
  const auto &entries = subnet.getEntries();

  const auto iMin = subnet.getInNum();
  const auto iMax = subnet.size() - subnet.getOutNum();

  for (size_t i = iMin; i < iMax; ++i) {
    const auto &cell = entries[i].cell;
    const auto newCellID = makeCell(netBuilder, subnet, i, inverse);

    inverse[i] = CellDescriptor{CellDescriptor::INNER, Link{}, newCellID};
    i += cell.more;
  }
}

/// Makes cells for the components' outputs.
static void makeCellsForOutputs(NetBuilder &netBuilder,
                                const Subnet &subnet,
                                const CellMapping &mapping,
                                InverseCellMapping &inverse,
                                InOutCellMapping &inout) {
  assert(subnet.getOutNum() == mapping.outputs.size());

  for (const auto &[oldLink, oldIdx] : mapping.outputs) {
    const auto oldSize = mapping.size;
    const auto newSize = subnet.size();
    assert((oldIdx + newSize) >= oldSize);

    // Outputs are located at the end.
    const size_t newIdx = (oldIdx + newSize) - oldSize;
    assert(newIdx + subnet.getOutNum() >= subnet.size());

    const auto link = subnet.getLink(newIdx, 0);
    const auto newSource = makeLinkEnd(netBuilder, link, inverse);

    const auto oldTargetID = oldLink.target.getCellID();
    const auto newTargetID = makeCell(netBuilder, oldTargetID, inout);
    const auto targetPort = oldLink.target.getPort();

    netBuilder.connect(newTargetID, targetPort, newSource);

    Link newLink{newSource, LinkEnd{newTargetID, targetPort}};
    inverse[newIdx] =
        CellDescriptor{CellDescriptor::OUTPUT, newLink, OBJ_NULL_ID};
  }
}

/// Adds the subnet to the composed net.
static void addSubnet(NetBuilder &netBuilder,
                      const SubnetID subnetID,
                      const CellMapping &mapping,
                      InOutCellMapping &inout) {
  const auto &subnet = Subnet::get(subnetID);
  InverseCellMapping inverse(subnet.size());

  makeCellsForInputs (netBuilder, subnet, mapping, inverse, inout);
  makeCellsForInners (netBuilder, subnet, mapping, inverse);
  makeCellsForOutputs(netBuilder, subnet, mapping, inverse, inout);
}

NetID NetDecomposer::compose(const Result &result) const {
  InOutCellMapping inout;
  inout.reserve(1024); // FIXME:

  NetBuilder netBuilder;

  const auto &inputs = result.inputs;
  for (auto i = inputs.begin(); i != inputs.end(); ++i) {
    makeCell(netBuilder, *i, inout);
  }

  const auto &outputs = result.outputs;
  for (auto i = outputs.begin(); i != outputs.end(); ++i) {
    makeCell(netBuilder, *i, inout);
  }

  const auto &subnets = result.subnets;
  for (const auto &subnet : subnets) {
    addSubnet(netBuilder, subnet.subnetID, subnet.mapping, inout);
  }

  return netBuilder.make();
}

} // namespace eda::gate::model
