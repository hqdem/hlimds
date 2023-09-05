//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/printer/dot.h" 
#include "gate/model2/printer/verilog.h"

#include <cassert>

namespace eda::gate::model {

NetPrinter &NetPrinter::getPrinter(NetFormat format) {
  switch (format) {
  case DOT:
    return DotPrinter::get();
  case VERILOG:
    return VerilogPrinter::get();
  default:
    return getDefaultPrinter();
  }
}

void NetPrinter::visitCells(
    std::ostream &out, const List<CellID> &cells, unsigned pass) {
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    onCell(out, *i, pass);
  }
}

void NetPrinter::visitCells(std::ostream &out, const Net &net, unsigned pass) {
  // TODO: Implement list view combining a number of lists.
  visitCells(out, net.getInputs(),     pass);
  visitCells(out, net.getOutputs(),    pass);
  visitCells(out, net.getCombCells(),  pass);
  visitCells(out, net.getFlipFlops(),  pass);
  visitCells(out, net.getSoftBlocks(), pass);
  visitCells(out, net.getHardBlocks(), pass);
}

void NetPrinter::visitLinks(
    std::ostream &out, const List<CellID> &cells, unsigned pass) {
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    const Cell::LinkList links = Cell::get(*i).getLinks();
    for (size_t j = 0; j < links.size(); ++j) {
      onLink(out, Link{links[j], LinkEnd{*i, static_cast<uint16_t>(j)}}, pass);
    }
  }
}

void NetPrinter::visitLinks(std::ostream &out, const Net &net, unsigned pass) {
  // TODO: Implement list view combining a number of lists.
  visitLinks(out, net.getInputs(),     pass);
  visitLinks(out, net.getOutputs(),    pass);
  visitLinks(out, net.getCombCells(),  pass);
  visitLinks(out, net.getFlipFlops(),  pass);
  visitLinks(out, net.getSoftBlocks(), pass);
  visitLinks(out, net.getHardBlocks(), pass);
}

void NetPrinter::print(
    std::ostream &out, const Net &net, const std::string &name) {
  onNetBegin(out, net, name);

  onInterfaceBegin(out);
  List<CellID> inputs = net.getInputs();
  for (auto i = inputs.begin(); i != inputs.end(); ++i) {
    onPort(out, *i);
  }
  List<CellID> outputs = net.getOutputs();
  for (auto i = outputs.begin(); i != outputs.end(); ++i) {
    onPort(out, *i);
  }
  onInterfaceEnd(out);

  onDefinitionBegin(out);
  for (auto pass : passes) {
    visitItems(out, net, pass);
  }
  onDefinitionEnd(out);

  onNetEnd(out, net, name);
}

} // namespace eda::gate::model
