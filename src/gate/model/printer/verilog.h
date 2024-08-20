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

/**
 * @brief Prints nets/subnets in Verilog.
 */
class VerilogPrinter final : public ModelPrinter,
                             public util::Singleton<VerilogPrinter> {
  friend class util::Singleton<VerilogPrinter>;

  VerilogPrinter(): ModelPrinter({{Pass::CELL, 0}, {Pass::CELL, 1}}) {}

  void onModelBegin(std::ostream &out, const std::string &name) override;
  void onModelEnd(std::ostream &out, const std::string &name) override;

  void onInterfaceBegin(std::ostream &out) override;
  void onInterfaceEnd(std::ostream &out) override;

  void onType(std::ostream &out, const CellType &cellType) override;

  void onPort(std::ostream &out, const CellInfo &cellInfo) override;

  void onCell(std::ostream &out,
              const CellInfo &cellInfo,
              const LinksInfo &linksInfo,
              unsigned pass) override;

private:
  bool isFirstPort{false};
};

} // namespace eda::gate::model
