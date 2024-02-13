//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
#include "gate/model/gnet.h"
#include "util/singleton.h"

#include <ostream>
#include <string>

namespace eda::gate::printer {

/// Printer to VCD format.
class VCDPrinter : public util::Singleton<VCDPrinter> {
  friend class util::Singleton<VCDPrinter>;

public:
  using GNet = eda::gate::model::GNet;
  using CheckerResult = eda::gate::debugger::CheckerResult;

  /**
   * \brief Creates a VCD file.
   * @param out Output stream.
   * @param net The net which the VCD file is based on.
   * @param counterEx Input values for the net.
   */
  void print(std::ostream &out,
             const GNet &net,
             const std::vector<bool> &values) const;

  /**
   * \brief Utility. Creates a VCD file.
   * @param filename Specifies the name of the VCD file.
   * @param net The net which the VCD file is based on.
   * @param counterEx Input values for the net.
   */
  void print(const std::string &filename,
             const GNet &net,
             const std::vector<bool> &values) const;

  /**
   * \brief Utility. Creates a VCD file.
   * @param filename Specifies the name of the VCD file.
   * @param net The net which the VCD file is based on.
   * @param res Equivalence checking result to get input values from.
   */
  void print(const std::string &filename,
             const GNet &net,
             const CheckerResult res) const;

  /// Path to the template.
  static inline const std::string TEMPLATE_PATH = 
    "src/data/ctemplate/vcd.tpl";

private:
  static inline const std::string DICTIONARY_NAME = "vcd";
  // Header
  static inline const std::string GEN_TIME = "GEN_TIME";
  static inline const std::string NET_ID = "NET_ID";
  static inline const std::string NET_NAME_PREFIX = "net_";
  // Variables
  static inline const std::string GATE_NAME_PREFIX = "g_";
  static inline const std::string GID = "GID";
  static inline const std::string VAR = "var_";
  static inline const std::string VARS = "VARS";
  static inline const std::string VAR_ID = "VAR_ID";
  // Values
  static inline const std::string VALUE = "VALUE";
  static inline const std::string VALUES = "VALUES";

  VCDPrinter() {}
};

} // namespace eda::gate::printer
