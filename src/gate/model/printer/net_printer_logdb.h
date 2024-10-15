//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/printer/net_printer_base.h"
#include "gate/model/subnet.h"
#include "util/singleton.h"

namespace eda::gate::model {

class NetPrinterLogDb final : public NetPrinter,
                              public util::Singleton<NetPrinterLogDb> {
  friend class util::Singleton<NetPrinterLogDb>;

  NetPrinterLogDb(): NetPrinter({{Pass::CELL, 0}}) {}

  void onCell(std::ostream &out,
              const CellInfo &cellInfo,
              const LinksInfo &linksInfo,
              unsigned pass) override {
  
    printCellType(out, cellInfo.type.get());

    for (const auto &linkInfo : linksInfo) {
      out << ' ';
      if (linkInfo.inv) out << '~';
      out << linkInfo.sourceInfo.cellInfo.originalID;
      if (linkInfo.sourceInfo.port) out << '.' << linkInfo.sourceInfo.port;
    }
    out << '\n';
  }

  void printCellType(std::ostream &out, const CellType &type) const {
    out << type.getName();
  }
};

} // namespace eda::gate::model
