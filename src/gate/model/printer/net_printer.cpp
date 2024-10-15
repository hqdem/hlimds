//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "net_printer.h"
#include "net_printer_debug.h"
#include "net_printer_dot.h"
#include "net_printer_logdb.h"
#include "net_printer_verilog.h"

namespace eda::gate::model {

static NetPrinter &getPrinter(Format format) {
  switch (format) {
  case DEBUG:
    return NetPrinterDebug::get();
  case DOT:
    return NetPrinterDot::get();
  case VERILOG:
    return NetPrinterVerilog::get();
  case LOGDB:
    return NetPrinterLogdb::get();
  default:
    return NetPrinterDebug::get();
  }
}

template <typename T>
static void printDesign(std::ostream &out,
                        const Format format,
                        const std::string &name,
                        const T &netOrSubnet,
                        const CellTypeID typeID) {
  auto &printer = getPrinter(format);
  printer.print(out, netOrSubnet, name, typeID);
}

template <typename T>
static void printDesign(std::ostream &out,
                        const Format format,
                        const T &netOrSubnet,
                        const CellTypeID typeID) {
  static constexpr auto defaultName = "Design";
  printDesign<T>(out, format, defaultName, netOrSubnet, typeID);
}

void print(std::ostream &out,
           const Format format,
           const std::string &name,
           const Net &net,
           const CellTypeID typeID) {
  printDesign<Net>(out, format, name, net, typeID);
}

void print(std::ostream &out,
           const Format format,
           const std::string &name,
           const Subnet &subnet,
           const CellTypeID typeID) {
  printDesign<Subnet>(out, format, name, subnet, typeID);
}

void print(std::ostream &out,
           const Format format,
           const Net &net,
           const CellTypeID typeID) {
  printDesign<Net>(out, format, net, typeID);
}

void print(std::ostream &out,
           const Format format,
           const Subnet &subnet,
           const CellTypeID typeID) {
  printDesign<Subnet>(out, format, subnet, typeID);
}

} // namespace eda::gate::model
