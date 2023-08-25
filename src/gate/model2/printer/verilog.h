//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/printer/printer.h"
#include "util/singleton.h"

#include <sstream>

namespace eda::gate::model {

class VerilogPrinter final : public NetPrinter,
                             public util::Singleton<VerilogPrinter> {
protected:
  void onNetBegin(std::ostream &out,
                  const Net &net,
                  const std::string &name) override {
    out << "module " << name;
  }

  void onNetEnd(std::ostream &out,
                const Net &net,
                const std::string &name) override {
    out << "endmodule" << " // module " << name << "\n";
  }

  void onInterfaceBegin(std::ostream &out) override {
    out << "(\n";
  }

  void onInterfaceEnd(std::ostream &out) override {
    out << ");\n";
  }

  void onDefinitionBegin(std::ostream &out) override {
    // Do nothing.
  }

  void onDefinitionEnd(std::ostream &out) override {
    // Do nothing.
  }

  void onInputPort(std::ostream &out, const CellID &cellID) override {
    out << "  input " << getName(cellID) << ",\n";
  }

  void onOutputPort(std::ostream &out, const CellID &cellID) override {
    out << "  output " << getName(cellID) << ",\n";
  }

  void onLink(std::ostream &out, const Link &link) override {
  }

  void onCell(std::ostream &out, const CellID &cellID) override {
  }

  void onCellType(std::ostream &out, const CellType &cellType) override {
  }

private:

  static std::string getName(const CellID &cellID) {
    std::stringstream ss;

    ss << Cell::get(cellID).getType().getName();
    ss << "_";
    ss << Cell::makeSID(cellID);

    return ss.str();
  }
};

} // namespace eda::gate::model
