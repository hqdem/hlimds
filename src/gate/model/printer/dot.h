//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/printer/printer.h"
#include "util/singleton.h"

namespace eda::gate::model {

/// Prints nets in dot notation.
class DotPrinter final : public ModelPrinter,
                         public util::Singleton<DotPrinter> {
  friend class util::Singleton<DotPrinter>;

  DotPrinter(): ModelPrinter({{Pass::CELL, 0}, {Pass::LINK, 0}}) {}

  void onModelBegin(std::ostream &out,
                    const std::string &name,
                    const CellTypeID typeID) override {
    out << "digraph " << name << " {\n";
  }

  void onModelEnd(std::ostream &out,
                  const std::string &name,
                  const CellTypeID typeID) override {
    out << "}\n";
  }

  void onCell(std::ostream &out,
              const CellInfo &cellInfo,
              const LinksInfo &linksInfo,
              unsigned pass) override {
    out << "  " << cellInfo.getName()
        << "[label=" << cellInfo.getType() <<  "];\n";
  }

  void onLink(std::ostream &out,
              const LinkInfo &linkInfo,
              unsigned pass) override {
    out << "  "
        << linkInfo.sourceInfo.getName() << " -> "
        << linkInfo.targetInfo.getName() << ";\n";
  }
};

} // namespace eda::gate::model
