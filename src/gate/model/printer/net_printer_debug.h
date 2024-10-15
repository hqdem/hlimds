//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "net_printer_base.h"
#include "util/singleton.h"

namespace eda::gate::model {

class NetPrinterDebug final : public NetPrinter,
                              public util::Singleton<NetPrinterDebug> {
  friend class util::Singleton<NetPrinterDebug>;

  NetPrinterDebug(): NetPrinter({{Pass::CELL, 0}}) {}

  void onCell(std::ostream &out,
              const CellInfo &cellInfo,
              const LinksInfo &linksInfo,
              unsigned pass) override {
    out << cellInfo.printingID << " <= " << cellInfo.getType();

    out << "(";

    bool comma = false;
    for (const auto &linkInfo : linksInfo) {
      if (comma) out << ", ";

      if (linkInfo.inv) out << "~";
      out << linkInfo.sourceInfo.cellInfo.printingID <<
      "." << linkInfo.sourceInfo.port;

      comma = true;
    }

    out << ");\n";
  }
};

} // namespace eda::gate::model
