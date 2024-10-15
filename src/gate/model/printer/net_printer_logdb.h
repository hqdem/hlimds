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

class NetPrinterLogdb final : public NetPrinter,
                              public util::Singleton<NetPrinterLogdb> {

  friend class util::Singleton<NetPrinterLogdb>;

  NetPrinterLogdb(): NetPrinter({{Pass::CELL, 0}}) {}

  void onCell(std::ostream &out,
              const CellInfo &cellInfo,
              const LinksInfo &linksInfo,
              unsigned pass) override {
  
    printCellType(out, cellInfo.type.get());

    for (const auto &linkInfo : linksInfo) {
      out << ' ';
      if (linkInfo.inv) out << '~';
      out << linkInfo.sourceInfo.cellInfo.cellIDs.first;
      if (linkInfo.sourceInfo.port) out << '.' << linkInfo.sourceInfo.port;
    }
    out << '\n';
  }

  void printCellType(std::ostream &out, const CellType &type) const {
         if (type.isIn()  ) out << "in"  ;
    else if (type.isOut() ) out << "out" ;
    else if (type.isZero()) out << "zero";
    else if (type.isOne() ) out << "one" ;
    else if (type.isBuf() ) out << "buf" ;
    else if (type.isAnd() ) out << "and" ;
    else if (type.isOr()  ) out << "or"  ;
    else if (type.isXor() ) out << "xor" ;
    else if (type.isMaj() ) out << "maj" ;
    else throw std::runtime_error("Unsupported cell type while printing\n");
  }
};

} // namespace eda::gate::model
