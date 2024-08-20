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

#include <unordered_map>

namespace eda::gate::model {

/**
 * @brief Prints nets/subnets in Verilog.
 */
class VerilogPrinter final : public ModelPrinter,
                             public util::Singleton<VerilogPrinter> {
  friend class util::Singleton<VerilogPrinter>;

  VerilogPrinter(): ModelPrinter({{Pass::CELL, 0}, {Pass::CELL, 1}}) {}

  void onModelBegin(
      std::ostream &out,
      const std::string &name,
      const CellTypeID typeID) override;

  void onModelEnd(
      std::ostream &out,
      const std::string &name,
      const CellTypeID typeID) override;

  void onInterfaceBegin(std::ostream &out) override;

  void onInterfaceEnd(std::ostream &out) override;

  void onType(std::ostream &out, const CellType &cellType) override;

  void onPort(
      std::ostream &out,
      const CellInfo &cellInfo,
      unsigned index) override;

  void onCell(
      std::ostream &out,
      const CellInfo &cellInfo,
      const LinksInfo &linksInfo,
      unsigned pass) override;

private:
  bool isOrigIface{false};
  CellTypeID typeID{OBJ_NULL_ID};

  /// Maps an input/output cell to the pin index.
  std::unordered_map<uint32_t, unsigned> pins;
};

} // namespace eda::gate::model
