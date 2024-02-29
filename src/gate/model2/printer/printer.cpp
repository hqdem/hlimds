//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/printer/dot.h"
#include "gate/model2/printer/simple.h" 
#include "gate/model2/printer/verilog.h"

#include <cassert>

namespace eda::gate::model {

ModelPrinter &ModelPrinter::getPrinter(Format format) {
  switch (format) {
  case SIMPLE:
    return SimplePrinter::get();
  case DOT:
    return DotPrinter::get();
  case VERILOG:
    return VerilogPrinter::get();
  default:
    return getDefaultPrinter();
  }
}

//===----------------------------------------------------------------------===//
// Net-related methods
//===----------------------------------------------------------------------===//

ModelPrinter::CellInfo ModelPrinter::getCellInfo(CellID cellID) {
  return CellInfo{Cell::get(cellID).getType(), Cell::makeSID(cellID)};
}

ModelPrinter::PortInfo ModelPrinter::getPortInfo(CellID cellID, uint16_t port) {
  return PortInfo(getCellInfo(cellID), port);
}

ModelPrinter::LinkInfo ModelPrinter::getLinkInfo(const LinkEnd &source,
                                                 const LinkEnd &target) {
  return LinkInfo {
      getPortInfo(source.getCellID(), source.getPort()),
      getPortInfo(target.getCellID(), target.getPort()),
      false
  }; 
}

ModelPrinter::LinksInfo ModelPrinter::getLinksInfo(CellID cellID) {
  const auto links = Cell::get(cellID).getLinks();

  LinksInfo linksInfo;
  linksInfo.reserve(links.size());

  for (uint16_t j = 0; j < links.size(); ++j) {
    const LinkEnd source(links[j]);
    const LinkEnd target(cellID, j);
   
    linksInfo.emplace_back(getLinkInfo(source, target));
  }

  return linksInfo;
}

void ModelPrinter::visitInputs(
    std::ostream &out, const Net &net) {
  const List<CellID> inputs = net.getInputs();
  for (auto i = inputs.begin(); i != inputs.end(); ++i) {
    onPort(out, getCellInfo(*i));
  }
}

void ModelPrinter::visitOutputs(
    std::ostream &out, const Net &net) {
  const List<CellID> outputs = net.getOutputs();
  for (auto i = outputs.begin(); i != outputs.end(); ++i) {
    onPort(out, getCellInfo(*i));
  }
}

void ModelPrinter::visitCells(
    std::ostream &out, const List<CellID> &cells, unsigned pass) {
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    onCell(out, getCellInfo(*i), getLinksInfo(*i), pass);
  }
}

void ModelPrinter::visitCells(
    std::ostream &out, const Net &net, unsigned pass) {
  // TODO: Implement list view combining a number of lists.
  visitCells(out, net.getInputs(),     pass);
  visitCells(out, net.getOutputs(),    pass);
  visitCells(out, net.getCombCells(),  pass);
  visitCells(out, net.getFlipFlops(),  pass);
  visitCells(out, net.getSoftBlocks(), pass);
  visitCells(out, net.getHardBlocks(), pass);
}

void ModelPrinter::visitLinks(
    std::ostream &out, const List<CellID> &cells, unsigned pass) {
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    const Cell::LinkList links = Cell::get(*i).getLinks();
    for (uint16_t j = 0; j < links.size(); ++j) {
      const LinkEnd source(links[j]);
      const LinkEnd target(*i, j);
      onLink(out, getLinkInfo(source, target), pass);
    }
  }
}

void ModelPrinter::visitLinks(
    std::ostream &out, const Net &net, unsigned pass) {
  // TODO: Implement list view combining a number of lists.
  visitLinks(out, net.getInputs(),     pass);
  visitLinks(out, net.getOutputs(),    pass);
  visitLinks(out, net.getCombCells(),  pass);
  visitLinks(out, net.getFlipFlops(),  pass);
  visitLinks(out, net.getSoftBlocks(), pass);
  visitLinks(out, net.getHardBlocks(), pass);
}

//===----------------------------------------------------------------------===//
// Subnet-related methods
//===----------------------------------------------------------------------===//

ModelPrinter::CellInfo ModelPrinter::getCellInfo(
    const Subnet &subnet, size_t idx) {
  const auto &entries = subnet.getEntries();
  const auto &cell = entries[idx].cell;

  return CellInfo{cell.getType(), idx};
}

ModelPrinter::PortInfo ModelPrinter::getPortInfo(
    const Subnet &subnet, size_t idx, uint16_t j) {
  return PortInfo{getCellInfo(subnet, idx), j};
}

ModelPrinter::LinkInfo ModelPrinter::getLinkInfo(
    const Subnet &subnet, size_t idx, uint16_t j) {
  const auto link = subnet.getLink(idx, j);

  return LinkInfo {
      PortInfo{getCellInfo(subnet, link.idx), static_cast<uint16_t>(link.out)},
      PortInfo{getCellInfo(subnet, idx), j},
      static_cast<bool>(link.inv)
  }; 
}

ModelPrinter::LinksInfo ModelPrinter::getLinksInfo(
    const Subnet &subnet, size_t idx) {
  const auto links = subnet.getLinks(idx);

  LinksInfo linksInfo;
  linksInfo.reserve(links.size());

  for (uint16_t j = 0; j < links.size(); ++j) {
    linksInfo.emplace_back(getLinkInfo(subnet, idx, j));
  }

  return linksInfo;
}

void ModelPrinter::visitInputs(
    std::ostream &out, const Subnet &subnet) {
  for (size_t i = 0; i < subnet.getInNum(); ++i) {
    onPort(out, getCellInfo(subnet, i));
  }
}

void ModelPrinter::visitOutputs(
    std::ostream &out, const Subnet &subnet) {
  for (size_t i = subnet.size() - subnet.getOutNum(); i < subnet.size(); ++i) {
    onPort(out, getCellInfo(subnet, i));
  }
}

void ModelPrinter::visitCells(
    std::ostream &out, const Subnet &subnet, unsigned pass) {
  const auto &entries = subnet.getEntries();
  for (size_t i = 0; i < subnet.size(); ++i) {
    const auto &cell = entries[i].cell;
    onCell(out, getCellInfo(subnet, i), getLinksInfo(subnet, i), pass);
    i += cell.more;
  }
}

void ModelPrinter::visitLinks(
    std::ostream &out, const Subnet &subnet, unsigned pass) {
  const auto &entries = subnet.getEntries();
  for (size_t i = 0; i < subnet.size(); ++i) {
    const auto &cell = entries[i].cell;
    for (uint16_t j = 0; j < cell.arity; ++j) {
      onLink(out, getLinkInfo(subnet, i, j), pass);
    }
    i += cell.more;
  }
}

} // namespace eda::gate::model
