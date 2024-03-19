//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/decomposer/net_decomposer.h"

#include <cassert>
#include <cstdint>
#include <iostream>
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

using CellVec = std::vector<CellID>;
using CellSet = std::unordered_set<CellID>;
using CellMap = NetDecomposer::CellMap;

using CellMapping = NetDecomposer::CellMapping;

/// Aggregates cell information.
struct CellInfo final {
  const CellID cellID;
  const Cell &cell;
  const CellType &type;
};

/// Gets information on the given cell.
inline CellInfo getCellInfo(CellID cellID) {
  const auto &cell = Cell::get(cellID);
  const auto &type = cell.getType();

  return CellInfo{cellID, cell, type};
}

/// Gets information on the given cell (link-end).
inline CellInfo getCellInfo(LinkEnd linkEnd) {
  return getCellInfo(linkEnd.getCellID());
}

//===----------------------------------------------------------------------===//
// Decompositor
//===----------------------------------------------------------------------===//

/// Prepares the link to be an input mapping key.
inline Link makeInputLink(const Link &link) {
  return Link{link.source.getCellID(), link.source.getPort(), 0, 0};
}

/// Prepares the link to be an output mapping key.
inline Link makeOutputLink(const Link &link) {
  return link;
}

/// Makes a subnet link.
inline Subnet::Link makeLink(size_t index, uint16_t port, bool inv) {
  const auto idx = static_cast<uint32_t>(index);
  const auto out = static_cast<uint8_t>(port);
  return Subnet::Link{idx, out, inv};
}

/// Makes a subnet link for the given net link-end.
inline Subnet::Link makeLink(LinkEnd source, const CellMapping &mapping) {
  const auto i = mapping.inners.find(source.getCellID());
  if (i != mapping.inners.end()) {
    const auto entry = i->second;
    return makeLink(entry.first, source.getPort(), entry.second);
  }

  const Link inputLink{source.getCellID(), source.getPort(), 0, 0};
  const auto j = mapping.inputs.find(inputLink);
  assert(j != mapping.inputs.end());

  return makeLink(j->second, 0, false);
}

/// Makes a subnet link list for the given net cell.
inline Subnet::LinkList makeLinkList(const Cell &cell,
                                     const CellMapping &mapping) {
  Subnet::LinkList links;
  for (const auto &link : cell.getLinks()) {
    links.push_back(makeLink(link, mapping));
  }
  return links;
}

/// Checks if the link is an input (a primary input or a block output).
inline bool isInputLink(const Link &link) {
  const auto info = getCellInfo(link.source);
  return info.type.isIn()
      || info.type.isLatch()
      || info.type.isDff()
      || info.type.isDffRs()
      || info.type.isHard()
      || info.type.isSoft(); // TODO: Option.
}

/// Stores the cell links to the given list.
inline void fillLinks(CellID cellID, LinkVec &result) {
  const auto &cell = Cell::get(cellID);
  const auto links = cell.getLinks();

  for (uint16_t j = 0; j != links.size(); ++j) {
    result.push_back(Link{links[j], LinkEnd{cellID, j}});
  }
}

/// Stores the cells links to the given list.
inline void fillLinks(const List<CellID> &cells, LinkVec &result) {
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    fillLinks(*i, result);
  }
}

/// Returns the cell links.
inline LinkVec getLinks(CellID cellID) {
  LinkVec links{};
  fillLinks(cellID, links);
  return links;
}

/// Returns the component outputs (primary outputs and block inputs).
inline LinkVec extractOutputs(const Net &net) {
  LinkVec result{};
  result.reserve(net.getCellNum());

  fillLinks(net.getOutputs(),    result);
  fillLinks(net.getFlipFlops(),  result);
  fillLinks(net.getSoftBlocks(), result); // TODO: Option.
  fillLinks(net.getHardBlocks(), result);

  return result;
}

/// Describes a subnet.
struct NetComponent final {
  /// Inputs are links of the form <(src-cell:src-port), (0:0)>,
  /// i.e. only sources matter.
  LinkSet inputs;
  /// Inner cells are just cells, not links (topologically sorted).
  CellVec inners;
  /// Outputs are links of the form <(src-cell:src-port), (dst-cell:dst-port)>,
  /// i.e. targets matter (especially flip-flops).
  LinkSet outputs;

  /// Resets the component state.
  void clear() {
    inputs.clear();
    inners.clear();
    outputs.clear();
  }

  /// Merges the component w/ this one.
  void merge(const NetComponent &r)  {
    inputs.insert(r.inputs.begin(), r.inputs.end());
    inners.insert(inners.end(), r.inners.begin(), r.inners.end());
    outputs.insert(r.outputs.begin(), r.outputs.end());
  }
};

/// Traveral stack entry.
struct NetTraversalEntry final {
  /// Checks if the entry corresponds to an input.
  bool isInput() const { return isInputLink(getLink()); }
  /// Checks if the entry corresponds to an output.
  bool isOutput() const { return cellID == OBJ_NULL_ID; }
  /// Checks if the entry is fully traversed.
  bool isPassed() const { return index >= links.size(); }
  /// Returns the current link of the entry.
  const Link &getLink() const { return links[index]; }

  const CellID cellID;
  const LinkVec links;
  size_t index;
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

    stack.push(NetTraversalEntry{OBJ_NULL_ID, extractOutputs(net), 0});
  }

  /// Checks if the traversal is completed.
  bool isCompleted() const { return stack.empty(); }
  /// Returns the top entry of the stack.
  NetTraversalEntry &top() { return stack.top(); }
  /// Pops the top entry from the stack.
  void pop() { stack.pop(); }

  /// Checks if the cell is new and pushes it to the stack.
  void push(CellID cellID) {
    const auto i = belongsTo.find(cellID);
    if (i != belongsTo.end()) {
      componentIndex = i->second;
      return;
    }
    const auto j = componentCells.find(cellID);
    if (j == componentCells.end()) {
      componentCells.insert(cellID);
      stack.push(NetTraversalEntry{cellID, getLinks(cellID), 0});
    }
  }

  /// Adds the input link to the current component.
  void addInput(const Link &link) {
    // Using a set avoids duplicates.
    component.inputs.insert(link);
  }

  /// Adds the inner cell to the current component.
  void addInner(CellID cellID) {
    component.inners.push_back(cellID);
  }

  /// Adds the output link to the current component.
  void addOutput(const Link &link) {
    component.outputs.insert(link);
  }

  /// Adds the previously constructed component to the list.
  void addComponent() {
    if (componentIndex < components.size()) {
      // Merge the current component w/ the existing one.
      components[componentIndex].merge(component);
    } else {
      // Add the component as a new one.
      components.push_back(component);
    }

    // Update the cell mapping.
    for (const auto &componentCell : componentCells) {
      belongsTo.insert({componentCell, componentIndex});
    }

    // Reset the component state (start building a new one).
    component.clear();
    componentCells.clear();
    componentIndex = components.size();
  }

  /// Increments the link index of the top entry.
  void nextLink() { stack.top().index++; }

  /// Maps cells to components.
  std::unordered_map<CellID, size_t> belongsTo;
  /// Stores the constructed components.
  std::vector<NetComponent> components;
  /// Component under construction.
  NetComponent component;
  /// Index of the currently constructed component.
  size_t componentIndex = 0;
  /// Stores the current component's inner cells.
  CellSet componentCells;
  /// Traversal stack (DFS from outputs to inputs).
  NetTraversalStack stack;
};

/// Returns the net components (connected subnets).
static std::vector<NetComponent> extractComponents(const Net &net) {
  NetTraversalContext context(net);

  while (true) {
    auto &entry = context.top();

    if (entry.isOutput() && entry.index > 0) {
      context.addComponent();

      if (entry.isPassed()) {
        context.pop();
        break;
      }
    }

    if (!entry.isPassed()) {
      const auto &link = entry.getLink();
      const auto isInput = entry.isInput();
      const auto isOutput = entry.isOutput();

      if (isOutput) {
        context.addOutput(makeOutputLink(link));
      }
      if (isInput) {
        context.addInput(makeInputLink(link));
      }

      context.nextLink();

      if (!isInput) {
        context.push(link.source.getCellID());
      }
    } else {
      // For topological ordering, add a cell right before popping.
      context.addInner(entry.cellID);
      context.pop();
    }
  }

  return context.components;
}

/// Makes a subnet for the given net component.
static SubnetID makeSubnet(const Net &net,
                           const NetComponent &component,
                           CellMapping &mapping) {
  SubnetBuilder subnetBuilder;

  for (const auto &input : component.inputs) {
    const auto info = getCellInfo(input.source);
    const auto link = info.type.isCombinational()
        ? subnetBuilder.addInput()
        : subnetBuilder.addInput(info.cellID.getSID());

    const auto inputLink = makeInputLink(input);
    mapping.inputs.insert({inputLink, link.idx});
  }

  for (const auto &inner : component.inners) {
    const auto info = getCellInfo(inner);
    const auto ilinks = makeLinkList(info.cell, mapping);

    const auto neg = info.type.isNegative();
    const auto sym = info.type.getSymbol();

    Subnet::Link olink;

    if (sym == BUF || sym == NOT) {
      olink = makeLink(info.cell.getLink(0), mapping);
    } else {
      olink = subnetBuilder.addCell((neg ? getNegSymbol(sym) : sym), ilinks);
    }

    const auto idx = olink.idx;
    const auto inv = olink.inv ^ neg;
    mapping.inners.insert({info.cellID, {idx, inv}});
  }

  for (const auto &output : component.outputs) {
    const auto info = getCellInfo(output.target);
    const auto ilink = makeLink(output.source, mapping);
    const auto olink = info.type.isCombinational()
        ? subnetBuilder.addOutput(ilink)
        : subnetBuilder.addOutput(ilink, info.cellID.getSID());

    const auto outputLink = makeOutputLink(output);
    mapping.outputs.insert({outputLink, olink.idx});
  }

  const auto subnetID = subnetBuilder.make();
  const auto &subnet = Subnet::get(subnetID);

  // Subnet size is required for proper composition.
  mapping.size = subnet.size();

  return subnetID;
}

void NetDecomposer::decompose(NetID netID,
                              std::vector<SubnetID> &subnets,
                              std::vector<CellMapping> &mapping) const {
  const auto &net = Net::get(netID);
  const auto components = extractComponents(net);

  assert(subnets.empty());
  subnets.reserve(components.size());

  assert(mapping.empty());
  mapping.reserve(components.size());

  for (const auto &component : components) {
    CellMapping subnetMapping;
    subnets.push_back(makeSubnet(net, component, subnetMapping));
    mapping.push_back(subnetMapping);
  }
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
using InOutMapping = std::unordered_map<CellID, CellID>;

/// Makes a link-end corresponding to the given subnet link.
inline LinkEnd makeLinkEnd(NetBuilder &netBuilder,
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
                       CellID oldCellID,
                       InOutMapping &inout) {
  const auto i = inout.find(oldCellID);

  // Already created.
  if (i != inout.end()) {
    return i->second;
  }

  const auto &oldCell = Cell::get(oldCellID);
  const Cell::LinkList invalidLinks(oldCell.getFanin());
  const auto newCellID = makeCell(oldCell.getTypeID(), invalidLinks);

  netBuilder.addCell(newCellID);
  inout.insert({oldCellID, newCellID});

  return newCellID;
}

/// Makes a new inner cell for the given subnet cell.
static CellID makeCell(NetBuilder &netBuilder,
                       const Subnet &subnet,
                       size_t idx,
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
                               InOutMapping &inout) {
  assert(subnet.getInNum() == mapping.inputs.size());

  for (const auto &[oldLink, oldIdx] : mapping.inputs) {
    // Inputs are located at the beginning.
    const auto newIdx = oldIdx;
    assert(newIdx < subnet.getInNum());

    const auto oldSourceID = oldLink.source.getCellID();
    const auto newSourceID = makeCell(netBuilder, oldSourceID, inout);

    Link newLink{newSourceID, oldLink.source.getPort(), 0, 0};
    inverse[newIdx] = CellDescriptor{CellDescriptor::INPUT, newLink, OBJ_NULL_ID};
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
                                InOutMapping &inout) {
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
    inverse[newIdx] = CellDescriptor{CellDescriptor::OUTPUT, newLink, OBJ_NULL_ID};
  }
}

/// Adds the subnet to the composed net.
static void addSubnet(NetBuilder &netBuilder,
                      SubnetID subnetID,
                      const CellMapping &mapping,
                      InOutMapping &inout) {
  const auto &subnet = Subnet::get(subnetID);
  InverseCellMapping inverse(subnet.size());

  makeCellsForInputs (netBuilder, subnet, mapping, inverse, inout);
  makeCellsForInners (netBuilder, subnet, mapping, inverse);
  makeCellsForOutputs(netBuilder, subnet, mapping, inverse, inout);
}

NetID NetDecomposer::compose(const std::vector<SubnetID> &subnets,
                             const std::vector<CellMapping> &mapping) const {
  assert(subnets.size() == mapping.size());

  NetBuilder netBuilder;

  InOutMapping inout;
  inout.reserve(1024); // FIXME:

  for (size_t i = 0; i < subnets.size(); ++i) {
    addSubnet(netBuilder, subnets[i], mapping[i], inout);
  }

  return netBuilder.make();
}

} // namespace eda::gate::model
