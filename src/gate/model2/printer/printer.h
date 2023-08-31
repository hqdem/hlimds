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

/// Supported net formats.
enum NetFormat {
  DOT,
  VERILOG
};

/// Base net printer class.
class NetPrinter {
public:
  static NetPrinter &getDefaultPrinter() {
    return getPrinter(DOT);
  }

  static NetPrinter &getPrinter(NetFormat format);

  /// Outputs the net w/ the specified name.
  void print(std::ostream &out, const Net &net, const std::string &name);
  /// Outputs the net w/ the default name.
  void print(std::ostream &out, const Net &net) { print(out, net, "Design"); }

protected:
  /// Describes a print pass.
  struct Pass {
    enum { LINK, CELL } type;
    unsigned num;
  };

  NetPrinter(const std::vector<Pass> passes): passes(passes) {}
  virtual ~NetPrinter() {}

  virtual void onNetBegin(
      std::ostream &out, const Net &net, const std::string &name) = 0;

  virtual void onNetEnd(
      std::ostream &out, const Net &net, const std::string &name) = 0;

  virtual void onInterfaceBegin(std::ostream &out) = 0;
  virtual void onInterfaceEnd(std::ostream &out) = 0;

  virtual void onPort(std::ostream &out, const CellID &cellID) = 0;

  virtual void onDefinitionBegin(std::ostream &out) = 0;
  virtual void onDefinitionEnd(std::ostream &out) = 0;

  virtual void onCell(
      std::ostream &out, const CellID &cellID, unsigned pass) = 0;
  virtual void onLink(
      std::ostream &out, const Link &link, unsigned pass) = 0;

private:
  void visitCells(std::ostream &out, const List<CellID> &cells, unsigned pass);
  void visitCells(std::ostream &out, const Net &net, unsigned pass);

  void visitLinks(std::ostream &out, const List<CellID> &cells, unsigned pass);
  void visitLinks(std::ostream &out, const Net &net, unsigned pass);

  void visitItems(std::ostream &out, const Net &net, const Pass &pass) {
    switch (pass.type) {
    case Pass::LINK:
      visitLinks(out, net, pass.num);
      break;
    case Pass::CELL:
      visitCells(out, net, pass.num);
      break;
    default:
      assert(false);
      break;
    }
  }

  const std::vector<Pass> passes;
};

inline std::ostream &operator <<(std::ostream &out, const Net &net) {
  NetPrinter::getDefaultPrinter().print(out, net);
  return out;
}

} // namespace eda::gate::model
