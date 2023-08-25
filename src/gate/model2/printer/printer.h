//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/cell.h"
#include "gate/model2/celltype.h"
#include "gate/model2/link.h"
#include "gate/model2/list.h"
#include "gate/model2/net.h"
#include "gate/model2/string.h"

#include <ostream>
#include <string>

namespace eda::gate::model {

enum NetFormat {
  VERILOG
};

class NetPrinter {
public:
  static NetPrinter &getPrinter(NetFormat format);
  static NetPrinter &getDefaultPrinter() { return getPrinter(VERILOG); }

  /// Outputs the net w/ the specified name.
  void print(std::ostream &out, const Net &net, const std::string &name);
  /// Outputs the net w/ the default name.
  void print(std::ostream &out, const Net &net) { print(out, net, "design"); }

protected:
  NetPrinter() {}

  virtual void onNetBegin(std::ostream &out,
                          const Net &net,
                          const std::string &name) = 0;

  virtual void onNetEnd(std::ostream &out,
                        const Net &net,
                        const std::string &name) = 0;

  virtual void onInterfaceBegin(std::ostream &out) = 0;
  virtual void onInterfaceEnd(std::ostream &out) = 0;

  virtual void onDefinitionBegin(std::ostream &out) = 0;
  virtual void onDefinitionEnd(std::ostream &out) = 0;

  virtual void onInputPort(std::ostream &out, const CellID &cellID) = 0;
  virtual void onOutputPort(std::ostream &out, const CellID &cellID) = 0;

  virtual void onLink(std::ostream &out, const Link &link) = 0;
  virtual void onCell(std::ostream &out, const CellID &cellID) = 0;

  virtual void onCellType(std::ostream &out, const CellType &cellType) = 0;

private:
  void visitLinks(const List<CellID> &cells);
  void visitCells(const List<CellID> &cells);
};

inline std::ostream &operator <<(std::ostream &out, const Net &net) {
  NetPrinter::getDefaultPrinter().print(out, net);
  return out;
}

} // namespace eda::gate::model
