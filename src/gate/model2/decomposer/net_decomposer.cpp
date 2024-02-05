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
#include <unordered_map>

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
};

inline CellInfo getCellInfo(CellID cellID) {
  const auto &cell = Cell::get(cellID);
  const auto &type = cell.getType();

  return CellInfo{cellID, cell, type};
}

inline CellInfo getCellInfo(LinkEnd linkEnd) {
  return getCellInfo(linkEnd.getCellID());
}

inline Subnet::Link makeLink(size_t index, uint16_t port) {
  const auto idx = static_cast<uint32_t>(index);
  const auto out = static_cast<uint8_t>(port);
  return Subnet::Link{idx, out, 0};
}

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

inline Subnet::LinkList makeLinkList(const Cell &cell,
                                     const CellMapping &mapping) {
  Subnet::LinkList links;
  for (const auto &link : cell.getLinks()) {
    links.push_back(makeLink(link, mapping));
  }
  return links;
}

static SubnetID makeSubnet(const Net &net, const NetComponent &component) {
  SubnetBuilder subnetBuilder;
  CellMapping mapping;

  for (const auto &input : component.inputs) {
    const auto info = getCellInfo(input.source);
    const auto index = info.type.isCombinational()
        ? subnetBuilder.addInput()
        : subnetBuilder.addInput(info.cellID.getSID());

    const Link link{info.cellID, input.source.getPort(), 0, 0};
    mapping.inputs.insert(LinkPair{link, index.idx});
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

    mapping.outputs.insert(LinkPair{output, index.idx});
  }

  return subnetBuilder.make();
}

struct NetTraversalEntry final {
  std::vector<CellID> inputs;
  size_t index;
  bool output; 
};

using NetTraversalStack = std::vector<NetTraversalEntry>;

std::vector<SubnetID> NetDecomposer::make(NetID netID) const {
  const auto &net = Net::get(netID);

  // TODO: Constructs components.
  std::vector<NetComponent> components;

  std::vector<SubnetID> subnets;
  subnets.reserve(components.size());

  for (const auto &component : components) {
    subnets.push_back(makeSubnet(net, component));
  }

  return subnets;
}

} // namespace eda::gate::model
