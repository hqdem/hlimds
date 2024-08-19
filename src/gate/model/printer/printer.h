//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/cell.h"
#include "gate/model/celltype.h"
#include "gate/model/link.h"
#include "gate/model/list.h"
#include "gate/model/net.h"
#include "gate/model/string.h"
#include "gate/model/subnet.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace eda::gate::model {

/// Base Net/Subnet printer class.
class ModelPrinter {
public:
  /// Supported output formats.
  enum Format {
    SIMPLE,
    DOT,
    VERILOG
  };

  static constexpr auto DefaultName = "Design";

  static ModelPrinter &getDefaultPrinter() {
    return getPrinter(SIMPLE);
  }

  static ModelPrinter &getPrinter(Format format);

  /// Prints the net w/ the specified name.
  template <typename T /* Net/Subnet */>
  void print(std::ostream &out, const T &model, const std::string &name) {
    cellIDs.clear();

    onModelBegin(out, name);

    onInterfaceBegin(out);
    visitInputs (out, model);
    visitOutputs(out, model);
    onInterfaceEnd(out);

    onDefinitionBegin(out);
    for (const auto pass : passes) {
      switch (pass.type) {
      case Pass::LINK:
        visitLinks(out, model, pass.num);
        break;
      case Pass::CELL:
        visitCells(out, model, pass.num);
        break;
      default:
        assert(false);
        break;
      }
    }
    onDefinitionEnd(out);

    onModelEnd(out, name);
  }

  /// Prints the net w/ the default name.
  template <typename T /* Net/Subnet */>
  void print(std::ostream &out, const T &model) {
    print(out, model, DefaultName);
  }

protected:
  /// Describes a print pass.
  struct Pass {
    enum { LINK, CELL } type;
    unsigned num;
  };

  /// Cell information.
  struct CellInfo final {
    CellInfo(const CellType &type, size_t cell):
        type(type), cell(cell) {}

    std::string getType() const {
      return type.get().getName();
    }

    std::string getName() const {
      std::stringstream ss;

      ss << getType();
      ss << "_";
      ss << cell;

      return ss.str();
    }

    const std::reference_wrapper<const CellType> type;
    const size_t cell;
  };

  /// Port information.
  struct PortInfo final {
    PortInfo(const CellInfo &cellInfo, uint16_t port):
        cellInfo(cellInfo), port(port) {}

    std::string getName() const {
      if (cellInfo.type.get().getOutNum() <= 1) {
        return cellInfo.getName();
      }

      std::stringstream ss;
      ss << cellInfo.getName();
      ss << "_";
      ss << port;

      return ss.str();
    }

    const CellInfo cellInfo;
    const uint16_t port;
  };

  /// Link information.
  struct LinkInfo final {
    LinkInfo(const PortInfo &sourceInfo, const PortInfo &targetInfo, bool inv):
        sourceInfo(sourceInfo), targetInfo(targetInfo), inv(inv) {}

    std::string getSourceName() const { return sourceInfo.getName(); }
    std::string getTargetName() const { return targetInfo.getName(); }

    const PortInfo sourceInfo;
    const PortInfo targetInfo;
    const bool inv;
  };

  /// Inputs information.
  using LinksInfo = std::vector<LinkInfo>;

  ModelPrinter(const std::vector<Pass> passes): passes(passes) {}
  virtual ~ModelPrinter() {}

  virtual void onModelBegin(std::ostream &out, const std::string &name) {}
  virtual void onModelEnd(std::ostream &out, const std::string &name) {}

  virtual void onInterfaceBegin(std::ostream &out) {}
  virtual void onInterfaceEnd(std::ostream &out) {}

  virtual void onDefinitionBegin(std::ostream &out) {}
  virtual void onDefinitionEnd(std::ostream &out) {}

  virtual void onPort(std::ostream &out,
                      const CellInfo &cellInfo) {}

  virtual void onCell(std::ostream &out,
                      const CellInfo &cellInfo,
                      const LinksInfo &linksInfo,
                      unsigned pass) {}

  virtual void onLink(std::ostream &out,
                      const LinkInfo &linkInfo,
                      unsigned pass) {}

private:
  const std::vector<Pass> passes;

  using OriginalID = uint64_t;
  using PrintingID = uint32_t;
  std::unordered_map<OriginalID, PrintingID> cellIDs;

  /// Unifies the input and output names in designs w/ the same interface.
  size_t getCellPrintingID(OriginalID cellID) {
    PrintingID cellPrintingID{0};
    if (const auto found = cellIDs.find(cellID); found != cellIDs.end()) {
      cellPrintingID = found->second;
    } else {
      cellPrintingID = cellIDs.size();
      cellIDs.emplace(cellID, cellPrintingID);
    }
    return cellPrintingID;
  }

  //----------------------------------------------------------------------------
  // Net-related methods
  //----------------------------------------------------------------------------
  CellInfo getCellInfo(CellID cellID);
  PortInfo getPortInfo(CellID cellID, uint16_t port);
  LinkInfo getLinkInfo(const LinkEnd &source, const LinkEnd &target);
  LinksInfo getLinksInfo(CellID cellID);

  void visitInputs(std::ostream &out, const Net &net);
  void visitOutputs(std::ostream &out, const Net &net);
  void visitCells(std::ostream &out, const List<CellID> &cells, unsigned pass);
  void visitCells(std::ostream &out, const Net &net, unsigned pass);
  void visitLinks(std::ostream &out, const List<CellID> &cells, unsigned pass);
  void visitLinks(std::ostream &out, const Net &net, unsigned pass);

  //----------------------------------------------------------------------------
  // Subnet-related methods
  //----------------------------------------------------------------------------
  CellInfo getCellInfo(const Subnet &subnet, size_t idx);
  PortInfo getPortInfo(const Subnet &subnet, size_t idx, uint16_t j);
  LinkInfo getLinkInfo(const Subnet &subnet, size_t idx, uint16_t j);
  LinksInfo getLinksInfo(const Subnet &subnet, size_t idx);

  void visitInputs(std::ostream &out, const Subnet &subnet);
  void visitOutputs(std::ostream &out, const Subnet &subnet);
  void visitCells(std::ostream &out, const Subnet &subnet, unsigned pass);
  void visitLinks(std::ostream &out, const Subnet &subnet, unsigned pass);
};

} // namespace eda::gate::model
