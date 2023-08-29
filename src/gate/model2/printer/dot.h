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

/// Prints nets in dot notation.
class DotPrinter final : public NetPrinter,
                         public util::Singleton<DotPrinter> {
  friend class util::Singleton<DotPrinter>;

  static std::string getType(const CellID &cellID) {
    return Cell::get(cellID).getType().getName();
  }

  static std::string getName(const CellID &cellID) {
    std::stringstream ss;

    ss << getType(cellID);
    ss << "_";
    ss << Cell::makeSID(cellID);

    return ss.str();
  }

  DotPrinter(): NetPrinter({{Pass::CELL, 0}, {Pass::LINK, 0}}) {}

  void onNetBegin(
      std::ostream &out, const Net &net, const std::string &name) override {
    out << "digraph " << name << " {\n";
  }

  void onNetEnd(
      std::ostream &out, const Net &net, const std::string &name) override {
    out << "}\n";
  }

  void onInterfaceBegin(std::ostream &out) override {
    // Do nothing.
  }

  void onInterfaceEnd(std::ostream &out) override {
    // Do nothing.
  }

  void onPort(std::ostream &out, const CellID &cellID) override {
    // Do nothing.
  }

  void onDefinitionBegin(std::ostream &out) override {
    // Do nothing.
  }

  void onDefinitionEnd(std::ostream &out) override {
    // Do nothing.
  }

  void onCell(std::ostream &out, const CellID &cellID, unsigned pass) override {
    out << "  " << getName(cellID)
        << "[label=" << getType(cellID) <<  "];\n";
  }

  void onLink(std::ostream &out, const Link &link, unsigned pass) override {
    out << "  "
        << getName(link.source.getCellID()) << " -> "
        << getName(link.target.getCellID()) << ";\n";
  }
};

} // namespace eda::gate::model
