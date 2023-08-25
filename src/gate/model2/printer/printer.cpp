//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/printer/printer.h"
#include "gate/model2/printer/verilog.h"

#include <cassert>

namespace eda::gate::model {

NetPrinter &NetPrinter::getPrinter(NetFormat format) {
  switch (format) {
  case VERILOG:
    return VerilogPrinter::get();
  default:
    return getDefaultPrinter();
  }
}

void NetPrinter::visitLinks(const List<CellID> &cells) {
  for (auto i = cells.begin(); i != cells.end(); i++) {
    const Cell &cell = Cell::get(*i);

  }
}

void NetPrinter::visitCells(const List<CellID> &cells) {
}

void NetPrinter::print(std::ostream &out,
                       const Net &net,
                       const std::string &name) {

  onNetBegin(out, net, name);

  onInterfaceBegin(out);

  List<CellID> inputs = net.getInputs();
  for (auto i = inputs.begin(); i != inputs.end(); ++i) {
    onInputPort(out, *i);
  }

  List<CellID> outputs = net.getOutputs();
  for (auto i = outputs.begin(); i != outputs.end(); ++i) {
    onOutputPort(out, *i);
  }

  onInterfaceEnd(out);

  onDefinitionBegin(out);
  onDefinitionEnd(out);

  onNetEnd(out, net, name);
}

} // namespace eda::gate::model
