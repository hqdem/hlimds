//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "util/singleton.h"

#include <ostream>
#include <string>

namespace eda::gate::printer {

class GateVerilogPrinter : public util::Singleton<GateVerilogPrinter> {
  friend class util::Singleton<GateVerilogPrinter>;

public:
  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;
  using LinkSet = GNet::LinkSet;

  void print(std::ostream &out, const GNet &net) const;

  static inline const std::string templatePath = "src/data/ctemplate/gate_verilog.tpl";

private:
  // Template file markers & constants
  static inline const std::string DICTIONARY_NAME = "gate_verilog";
  static inline const std::string NET_NAME_PREFIX = "net_";
  // Header
  static inline const std::string GEN_TIME = "GEN_TIME";
  static inline const std::string MODULE_NAME = "MODULE_NAME";
  // Module inputs
  static inline const std::string INS = "INS";
  static inline const std::string INPUT = "INPUT";
  static inline const std::string SEPARATOR = "SEP";
  // Module outputs
  static inline const std::string OUTS = "OUTS";
  static inline const std::string OUTPUT = "OUTPUT";
  // Wires
  static inline const std::string WIRE_PREFIX = "wire_";
  static inline const std::string WIRES = "WIRES";
  static inline const std::string WIRE_NAME = "WIRE_NAME";
  // Assignments
  static inline const std::string ASSIGNS = "ASSIGNS";
  static inline const std::string LHS = "LHS";
  static inline const std::string RHS = "RHS";
  // Gates
  static inline const std::string GATE_NAME_PREFIX = "gate_";
  static inline const std::string GATES = "GATES";
  static inline const std::string GATE_TYPE = "GATE_TYPE";
  static inline const std::string GATE_NAME = "GATE_NAME";
  static inline const std::string GATE_OUT = "GATE_OUT";
  static inline const std::string G_INS = "G_INS";
  static inline const std::string GATE_IN = "GATE_IN";
  
  inline std::string wire(const uint32_t id) const {
    return WIRE_PREFIX + std::to_string(id);
  }
};

} // namespace eda::gate::printer
