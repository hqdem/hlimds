//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/printer/dot.h"
#include "gate/model/printer/simple.h" 
#include "gate/model/printer/verilog.h"

#include <cassert>
#include <set>

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
  const auto cellPrintingID = getCellPrintingID(cellID);
  return CellInfo{Cell::get(cellID).getType(), cellPrintingID};
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

void ModelPrinter::visitTypes(std::ostream &out, const List<CellID> &cells) {
  std::set<CellTypeID> typeIDs;
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    const auto &cell = Cell::get(*i);
    typeIDs.insert(cell.getTypeID());
  }
  for (const auto &typeID : typeIDs) {
    onType(out, CellType::get(typeID));
  }
}
 
void ModelPrinter::visitTypes(std::ostream &out, const Net &net) {
  // TODO: Implement list view combining a number of lists.
  visitTypes(out, net.getInputs());
  visitTypes(out, net.getOutputs());
  visitTypes(out, net.getCombCells());
  visitTypes(out, net.getFlipFlops());
  visitTypes(out, net.getSoftBlocks());
  visitTypes(out, net.getHardBlocks());
}

void ModelPrinter::visitInputs(std::ostream &out, const Net &net) {
  const List<CellID> inputs = net.getInputs();
  unsigned index{0};
  for (auto i = inputs.begin(); i != inputs.end(); ++i) {
    onPort(out, getCellInfo(*i), index++);
  }
}

void ModelPrinter::visitOutputs(std::ostream &out, const Net &net) {
  const List<CellID> outputs = net.getOutputs();
  unsigned index{net.getInNum()};
  for (auto i = outputs.begin(); i != outputs.end(); ++i) {
    onPort(out, getCellInfo(*i), index++);
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

  const auto cellPrintingID = getCellPrintingID(idx);
  return CellInfo{cell.getType(), cellPrintingID};
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

void ModelPrinter::visitTypes(std::ostream &out, const Subnet &subnet) {
  std::set<CellTypeID> typeIDs;
  for (size_t i = 0; i < subnet.getCellNum(); ++i) {
    const auto &cell = subnet.getCell(i);
    typeIDs.insert(cell.getTypeID());
    i += cell.more;
  }
  for (const auto &typeID : typeIDs) {
    onType(out, CellType::get(typeID));
  }
}

void ModelPrinter::visitInputs(std::ostream &out, const Subnet &subnet) {
  for (size_t i = 0; i < subnet.getInNum(); ++i) {
    onPort(out, getCellInfo(subnet, i), i);
  }
}

void ModelPrinter::visitOutputs(std::ostream &out, const Subnet &subnet) {
  const auto cellOffset = subnet.size() - subnet.getOutNum();
  const auto portOffset = subnet.getInNum();

  for (size_t i = 0; i < subnet.getOutNum(); ++i) {
    onPort(out, getCellInfo(subnet, cellOffset + i), portOffset + i);
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
