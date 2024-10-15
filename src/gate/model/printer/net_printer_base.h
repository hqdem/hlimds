//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/net.h"
#include "gate/model/subnet.h"

#include <fmt/format.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace eda::gate::model {

/**
 * @brief Base Net/Subnet printer class.
 */
class NetPrinter {
  using OriginalID = uint64_t;
  using PrintingID = uint32_t;

public:
  /// Cell information.
  struct CellInfo final {
    CellInfo(const CellType &type,
             OriginalID originalID,
             PrintingID printingID):
        type(type), originalID(originalID), printingID(printingID) {}

    std::string getType() const {
      return type.get().getName();
    }

    std::string getName() const {
      return fmt::format("{}_{}", getType(), printingID);
    }

    const std::reference_wrapper<const CellType> type;
    const OriginalID originalID;
    const PrintingID printingID;
  };

  /// Port information.
  struct PortInfo final {
    PortInfo(const CellInfo &cellInfo, uint16_t port):
        cellInfo(cellInfo), port(port) {}

    std::string getName() const {
      if (cellInfo.type.get().getOutNum() <= 1) {
        return cellInfo.getName();
      }

      return fmt::format("{}_{}", cellInfo.getName(), port);
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

  static constexpr auto DefaultName = "Design";

  /// Prints the net w/ the specified name.
  template <typename T /* Net/Subnet */>
  void print(std::ostream &out, const T &model, const std::string &name,
             const CellTypeID typeID = OBJ_NULL_ID) {
    visitTypes(out, model);

    cellIDs.clear();
    onModelBegin(out, name, typeID);

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

    onModelEnd(out, name, typeID);
  }

  /// Prints the net w/ the default name.
  template <typename T /* Net/Subnet */>
  void print(std::ostream &out, const T &model,
             const CellTypeID typeID = OBJ_NULL_ID) {
    print(out, model, DefaultName, typeID);
  }

protected:
  /// Describes a print pass.
  struct Pass {
    enum { LINK, CELL } type;
    unsigned num;
  };

  NetPrinter(const std::vector<Pass> passes): passes(passes) {}
  virtual ~NetPrinter() {}

  virtual void onModelBegin(std::ostream &out,
                            const std::string &name,
                            const CellTypeID typeID) {}
  virtual void onModelEnd(std::ostream &out,
                          const std::string &name,
                          const CellTypeID typeID) {}

  virtual void onInterfaceBegin(std::ostream &out) {}
  virtual void onInterfaceEnd(std::ostream &out) {}

  virtual void onDefinitionBegin(std::ostream &out) {}
  virtual void onDefinitionEnd(std::ostream &out) {}

  virtual void onType(std::ostream &out,
                      const CellType &cellType) {}

  virtual void onPort(std::ostream &out,
                      const CellInfo &cellInfo,
                      unsigned index) {}

  virtual void onCell(std::ostream &out,
                      const CellInfo &cellInfo,
                      const LinksInfo &linksInfo,
                      unsigned pass) {}

  virtual void onLink(std::ostream &out,
                      const LinkInfo &linkInfo,
                      unsigned pass) {}

private:
  const std::vector<Pass> passes;
  std::unordered_map<OriginalID, PrintingID> cellIDs;

  /// Unifies the input and output names in designs w/ the same interface.
  PrintingID getCellPrintingID(OriginalID cellID) {
    PrintingID cellPrintingID{0};
    if (const auto found = cellIDs.find(cellID); found != cellIDs.end()) {
      cellPrintingID = found->second;
    } else {
      cellPrintingID = cellIDs.size();
      cellIDs.emplace(cellID, cellPrintingID);
    }
    return cellPrintingID;
  }

  //===--------------------------------------------------------------------===//
  // Net-related methods
  //===--------------------------------------------------------------------===//

  CellInfo getCellInfo(CellID cellID);
  PortInfo getPortInfo(CellID cellID, uint16_t port);
  LinkInfo getLinkInfo(const LinkEnd &source, const LinkEnd &target);
  LinksInfo getLinksInfo(CellID cellID);

  void visitTypes(std::ostream &out, const List<CellID> &cells);
  void visitTypes(std::ostream &out, const Net &net);
  void visitInputs(std::ostream &out, const Net &net);
  void visitOutputs(std::ostream &out, const Net &net);
  void visitCells(std::ostream &out, const List<CellID> &cells, unsigned pass);
  void visitCells(std::ostream &out, const Net &net, unsigned pass);
  void visitLinks(std::ostream &out, const List<CellID> &cells, unsigned pass);
  void visitLinks(std::ostream &out, const Net &net, unsigned pass);

  //===--------------------------------------------------------------------===//
  // Subnet-related methods
  //===--------------------------------------------------------------------===//

  CellInfo getCellInfo(const Subnet &subnet, size_t idx);
  PortInfo getPortInfo(const Subnet &subnet, size_t idx, uint16_t j);
  LinkInfo getLinkInfo(const Subnet &subnet, size_t idx, uint16_t j);
  LinksInfo getLinksInfo(const Subnet &subnet, size_t idx);

  void visitTypes(std::ostream &out, const Subnet &subnet);
  void visitInputs(std::ostream &out, const Subnet &subnet);
  void visitOutputs(std::ostream &out, const Subnet &subnet);
  void visitCells(std::ostream &out, const Subnet &subnet, unsigned pass);
  void visitLinks(std::ostream &out, const Subnet &subnet, unsigned pass);
};

} // namespace eda::gate::model
