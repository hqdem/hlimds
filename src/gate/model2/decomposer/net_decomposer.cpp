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
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eda::gate::model {

using LinkList = std::vector<Link>;
using LinkPair = std::pair<Link, size_t>;
using CellList = std::vector<CellID>;
using CellPair = std::pair<CellID, size_t>;

/// Maps net cells/links to subnet cell indices.
struct CellMapping final {
  std::unordered_map<Link, size_t> inputs;
  std::unordered_map<CellID, size_t> inners;
  std::unordered_map<Link, size_t> outputs;
};

/// Aggregates cell information.
struct CellInfo final {
  const CellID cellID;
  const Cell &cell;
  const CellType &type;
};

/// Describes a subnet.
struct NetComponent final {
  /// Inputs are links of the form <(src-cell:src-port), (0:0)>,
  /// i.e. only sources matter.
  LinkList inputs;
  /// Inner cells are just cells, not links.
  CellList inners;
  /// Outputs are links of the form <(src-cell:src-port), (dst-cell:dst-port)>,
  /// i.e. targets matter (especially flip-flops).
  LinkList outputs;

  /// Resets the state.
  void clear() {
    inputs.clear();
    inners.clear();
    outputs.clear();
  }

  /// Merges the net component w/ this one.
  void merge(const NetComponent &r)  {
    inputs.insert(inputs.end(), r.inputs.begin(), r.inputs.end());
    inners.insert(inners.end(), r.inners.begin(), r.inners.end());
    outputs.insert(outputs.end(), r.outputs.begin(), r.outputs.end());
  }
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

/// Prepares the link to be an input mapping key.
inline Link makeInputLink(const Link &link) {
  return Link{link.source.getCellID(), link.source.getPort(), 0, 0};
}

/// Prepares the link to be an output mapping key.
inline Link makeOutputLink(const Link &link) {
  return link;
}

/// Makes a subnet link.
inline Subnet::Link makeLink(size_t index, uint16_t port) {
  const auto idx = static_cast<uint32_t>(index);
  const auto out = static_cast<uint8_t>(port);
  return Subnet::Link{idx, out, 0};
}

/// Makes a subnet link for the given net link-end.
inline Subnet::Link makeLink(LinkEnd source, const CellMapping &mapping) {
  const auto i = mapping.inners.find(source.getCellID());
  if (i != mapping.inners.end()) {
    return makeLink(i->second, source.getPort());
  }

  const Link link{source.getCellID(), source.getPort(), 0, 0};
  const auto j = mapping.inputs.find(link);
  assert(j != mapping.inputs.end());

  return makeLink(j->second, 0);
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

/// Makes a subnet for the given net component.
static SubnetID makeSubnet(const Net &net, const NetComponent &component) {
  SubnetBuilder subnetBuilder;
  CellMapping mapping;

  for (const auto &input : component.inputs) {
    const auto info = getCellInfo(input.source);
    const auto index = info.type.isCombinational()
        ? subnetBuilder.addInput()
        : subnetBuilder.addInput(info.cellID.getSID());

    const auto inputLink = makeInputLink(input);
    mapping.inputs.insert(LinkPair{inputLink, index.idx});
  }

  for (const auto &inner : component.inners) {
    const auto info = getCellInfo(inner);
    const auto links = makeLinkList(info.cell, mapping);
    const auto index = subnetBuilder.addCell(info.type.getSymbol(), links);

    mapping.inners.insert(CellPair{info.cellID, index.idx});
  }

  for (const auto &output : component.outputs) {
    const auto info = getCellInfo(output.target);
    const auto link = makeLink(output.source, mapping);
    const auto index = info.type.isCombinational()
        ? subnetBuilder.addOutput(link)
        : subnetBuilder.addOutput(link, info.cellID.getSID());

    const auto outputLink = makeOutputLink(output);
    mapping.outputs.insert(LinkPair{outputLink, index.idx});
  }

  return subnetBuilder.make();
}

/// Entry of DFS traveral stack.
struct NetTraversalEntry final {
  const CellID cellID;
  const LinkList links;
  size_t index;
};

/// DFS traversal stack.
using NetTraversalStack =
    std::stack<NetTraversalEntry, std::vector<NetTraversalEntry>>;

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
inline void fillLinks(CellID cellID, LinkList &result) {
  const auto &cell = Cell::get(cellID);
  const auto links = cell.getLinks();

  for (uint16_t j = 0; j != links.size(); ++j) {
    result.push_back(Link{links[j], LinkEnd{cellID, j}});
  }
}

/// Stores the cells links to the given list.
inline void fillLinks(const List<CellID> &cells, LinkList &result) {
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    fillLinks(*i, result);
  }
}

/// Returns the cell links.
inline LinkList getLinks(CellID cellID) {
  LinkList links{};
  fillLinks(cellID, links);
  return links;
}

/// Returns the component outputs (primary outputs and block inputs).
inline LinkList extractOutputs(const Net &net) {
  LinkList result{};
  result.reserve(net.getCellNum());

  fillLinks(net.getOutputs(),    result);
  fillLinks(net.getFlipFlops(),  result);
  fillLinks(net.getSoftBlocks(), result); // TODO: Option.
  fillLinks(net.getHardBlocks(), result);

  return result;
}

/// Returns the net components (connected subnets).
static std::vector<NetComponent> extractComponents(const Net &net) {
  // Maps cells to net components.
  std::unordered_map<CellID, size_t> belongsTo;
  belongsTo.reserve(net.getCellNum());

  // Stores the constructed components.
  std::vector<NetComponent> components;
  components.reserve(1024*1024);

  // DFS from outputs to inputs.
  NetTraversalStack stack;
  const auto outputs = extractOutputs(net);
  stack.push(NetTraversalEntry{OBJ_NULL_ID, outputs, 0});

  // Current net component.
  NetComponent component;
  size_t componentIndex = 0;
  std::unordered_set<CellID> componentCells;
  componentCells.reserve(1024*1024);

  while (true) {
    auto &entry = stack.top();
    const auto isFinished = (entry.index >= entry.links.size());

    if (entry.cellID == OBJ_NULL_ID && entry.index > 0) {
      // Add the previously constructed net component to the list.
      if (componentIndex < components.size()) {
        // Merge the net component w/ the existing one.
        components[componentIndex].merge(component);
      } else {
        // Add the net component as a new one.
        components.push_back(component);
      }

      // Update the cell map.
      for (const auto &componentCell : componentCells) {
        belongsTo.insert({componentCell, componentIndex});
      }

      // Reset the current net component.
      component.clear();
      componentCells.clear();
      componentIndex = components.size();

      if (isFinished) {
        stack.pop();
        break;
      }
    }

    // Go forward as far as possible.
    if (!isFinished) {
      const auto &link = entry.links[entry.index];

      if (entry.cellID == OBJ_NULL_ID) {
        // Add the output to the net component (promptly).
        component.outputs.push_back(link);
      } else if (isInputLink(link)) {
        // Add the input to the net component.
        component.inputs.push_back(link);
      } else {
        // Check if the cell belongs to a previously traversed component.
        const auto cellID = link.source.getCellID();
        const auto cellIt = belongsTo.find(cellID);

        if (cellIt != belongsTo.end()) {
          // Set the component index and stop moving forward.
          componentIndex = cellIt->second;
        } else {
          // Move forward.
          componentCells.insert(cellID);
          stack.push(NetTraversalEntry{cellID, getLinks(cellID), 0});
        }
      }

      // Next time choose the next link.
      entry.index++; 
    } else {
      // Append the inner cell to the current net component.
      component.inners.push_back(entry.cellID);
      // Step back.
      stack.pop();
    }
  }

  return components;
}

std::vector<SubnetID> NetDecomposer::make(NetID netID) const {
  const auto &net = Net::get(netID);
  const auto components = extractComponents(net);

  std::vector<SubnetID> subnets;
  subnets.reserve(components.size());

  for (const auto &component : components) {
    subnets.push_back(makeSubnet(net, component));
  }

  return subnets;
}

} // namespace eda::gate::model
