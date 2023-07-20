//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/gate_verilog_parser.h"

namespace eda::gate::parser::verilog {

  using GateId = eda::gate::model::GNet::GateId;
  using GateSymbol = eda::gate::model::GateSymbol;
  using ::lorina::text_diagnostics;
  using ::lorina::diagnostic_engine;
  using ::lorina::return_code;

  GateVerilogParser::GateVerilogParser(std::string name) {
    data->netName = std::move(name);
  }

  GateVerilogParser::~GateVerilogParser() {
    delete data;
  }

  eda::gate::model::GNet *GateVerilogParser::getGnet() { return data->gnet; }

  void GateVerilogParser::on_module_header(const std::string &moduleName,
                                           const std::vector<std::string> &inputs)
  const {
    data->startParse = (moduleName + ".v") == data->netName;
  }

  void GateVerilogParser::on_inputs(const std::vector<std::string> &inputs,
                                    std::string const &size) const {
    if (!data->startParse) {
      reportNameError();
      return;
    }
    for (const std::string &input: inputs) {
      std::string in = "#" + input;
      auto &gate = data->gates[in];
      gate.id = data->gnet->newGate();
      data->gIds.emplace(gate.id, data->gates.size() - 1);
      gate.kind = GateSymbol::IN;
      data->links[input] = ParserData::LinkData{in, {""}};
    }
  }

  void GateVerilogParser::on_outputs(const std::vector<std::string> &outputs,
                                     const std::string &size) const {
    if (!data->startParse) {
      reportNameError();
      return;
    }
    data->outputs = std::vector<std::string>(outputs.begin(), outputs.end());
  }

  void GateVerilogParser::on_wires(const std::vector<std::string> &wires,
                                   std::string const &size) const {
    if (!data->startParse) {
      reportNameError();
      return;
    }
    for (const auto &name: wires) {
      auto &gates = data->links[name];
      // There are 2 inputs as a rule.
      gates.sources.reserve(2);
    }
  }

  void GateVerilogParser::on_module_instantiation(
          std::string const &moduleName,
          std::vector<std::string> const &params,
          std::string const &instName,
          std::vector<std::pair<std::string, std::string>> const &args) const {
    if (!data->startParse) {
      reportNameError();
      return;
    }
    auto &gateData = data->gates[instName];
    gateData.id = data->gnet->newGate();

    data->gIds.emplace(gateData.id, data->gates.size() - 1);
    gateData.kind = symbol(moduleName);

    insertLink(args[0].second, instName, false);
    for (size_t i = 1; i < args.size(); ++i) {
      insertLink(args[i].second, instName, true);
    }
  }

  void GateVerilogParser::on_assign(const std::string &lhs,
                                    const std::pair<std::string, bool> &rhs) const {
    if (!data->startParse) {
      reportNameError();
      return;
    }

    std::string instName;
    GateSymbol gateSymbol;

    if (rhs.first.find('\'') != std::string::npos) {
      assert(rhs.first.size() == 4 && rhs.first.rfind("1'b", 0) == 0 &&
             "Parser can only handle such type of constant : \"1'bx\".");
      if (rhs.first[3] == '1') {
        instName = "1";
        gateSymbol = GateSymbol::ONE;
      } else {
        // Parser can only handle constant with 1 or 0.
        assert(rhs.first[3] == '0' &&
               "Parser can only handle constant with 1 or 0.");
        instName = "0";
        gateSymbol = GateSymbol::ZERO;
      }
    } else {
      instName = rhs.first + "->" + lhs;
      gateSymbol = GateSymbol::NOP;
    }

    auto gateDataIt = data->gates.find(instName);
    auto *gateData = &gateDataIt->second;

    if (gateDataIt == data->gates.end()) {
      gateData = &data->gates[instName];
      gateData->id = data->gnet->newGate();
      data->gIds.emplace(gateData->id, data->gates.size() - 1);
      insertLink(rhs.first, instName, true);
    }

    gateData->kind = gateSymbol;

    insertLink(lhs, instName, false);
  }

  void GateVerilogParser::on_endmodule() const {
    if (!data->startParse) {
      reportNameError();
      return;
    }
    // Collect links to make inputs arrays.
    for (const auto &[name, links]: data->links) {
      auto source = data->gates.find(links.target);

      for (size_t i = 0; i < links.sources.size(); ++i) {
        auto target = data->gates.find(links.sources[i]);
        if (source != data->gates.end() && target != data->gates.end()) {
          target->second.inputs.push_back(source->second.id);
        }
      }
    }
    // All gates are created - modifying them.
    for (const auto &[name, gateData]: data->gates) {
      std::vector<eda::base::model::Signal<GateId>> signals;
      signals.reserve(gateData.inputs.size());

      for (auto input: gateData.inputs) {
        signals.emplace_back(eda::base::model::Event::ALWAYS, input);
      }

      data->gnet->setGate(gateData.id, gateData.kind, signals);
    }

    // Creating output gates.
    for (const auto &output: data->outputs) {
      const auto &preOutGate = data->gates[data->links[output].target];
      data->gnet->addOut(preOutGate.id);
    }
  }

  void GateVerilogParser::insertLink(const std::string &name,
                                     const std::string &instName,
                                     bool out) const {
    auto &link = data->links[name];
    if (out) {
      link.sources.emplace_back(instName);
    } else {
      assert(link.target.empty());
      link.target = instName;
    }
  }

  GateSymbol GateVerilogParser::symbol(const std::string &s) const {
    if (s == "not") {
      return GateSymbol::NOT;
    } else if (s == "or") {
      return GateSymbol::OR;
    } else if (s == "xor") {
      return GateSymbol::XOR;
    } else if (s == "nand") {
      return GateSymbol::NAND;
    } else if (s == "nor") {
      return GateSymbol::NOR;
    } else if (s == "xnor") {
      return GateSymbol::XNOR;
    } else if (s == "and") {
      return GateSymbol::AND;
    }
    return GateSymbol::NOP;
  }

  eda::gate::model::GNet *getNet(const std::string &path,
                                 const std::string &netName) {
    text_diagnostics consumer;
    diagnostic_engine diag(&consumer);
    GateVerilogParser parser(netName);

    return_code result = read_verilog(path, parser, &diag);

    CHECK(result == return_code::success) << "File was not read successfully"
                                          << std::endl;

    return parser.getGnet();
  }

  void GateVerilogParser::reportNameError() const {
    std::cerr << "Non-equal names of parsed & expected module!" << std::endl;
  }

} // namespace eda::gate::parser::verilog
