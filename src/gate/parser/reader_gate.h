//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <fstream>

#include <lorina/verilog.hpp>

#include "gate/model/gnet.h"

class ReaderGate : public lorina::verilog_reader {
private:
  struct GateData {
    std::vector<eda::gate::model::GNet::GateId> inputs;
    eda::gate::model::GNet::GateId id;
    eda::gate::model::GateSymbol kind;
  };

  std::unordered_map<std::string, GateData> gates;
  // Wire name / <source, target>
  std::unordered_map<std::string, std::vector<std::string>> links;

  std::string netName;
  bool startParse = false;
  eda::gate::model::GNet gnet;

public:
  ReaderGate(std::string name) : netName(std::move(name)) {}

  eda::gate::model::GNet *getGnet() { return &gnet; }

  /*! \brief Callback method for parsed module.
   *
   * \param moduleName Name of the module
   * \param inouts Container for input and output names
   */
  void on_module_header(const std::string &moduleName,
                        const std::vector<std::string> &inputs) override {
    startParse = moduleName == netName;
  }

  /*! \brief Callback method for parsed inputs.
   *
   * \param inputs Input names
   * \param size Size modifier
   */
  void on_inputs(const std::vector<std::string> &inputs,
                 std::string const &size = "") override {
    if (startParse) {
      for (const std::string &input : inputs) {
        std::string in = "#" + input;
        auto &gate = gates[in];
        gate.id = gnet.newGate();
        gate.kind = eda::gate::model::GateSymbol::NOP;
        links[input] = {in, ""};
      }
    }
  }

  /*! \brief Callback method for parsed wires.
   *
   * \param wires Wire names
   * \param size Size modifier
   */
  void on_wires(const std::vector<std::string> &wires,
                std::string const &size = "") override {
    if (startParse) {
      for (auto &name : wires) {
        links[name].resize(2);
      }
    }
  }

  /*! \brief Callback method for parsed module instantiation of form `NAME
   * #(P1,P2) NAME(.SIGNAL(SIGANL), ..., .SIGNAL(SIGNAL));`
   *
   * \param moduleName Name of the module
   * \param params List of parameters
   * \param instName Name of the instantiation
   * \param args List (a_1,b_1), ..., (a_n,b_n) of name pairs, where
   *             a_i is a name of a signals in moduleName and b_i is a name of
   * a signal in instName.
   */
  virtual void on_module_instantiation(
      std::string const &moduleName, std::vector<std::string> const &params,
      std::string const &instName,
      std::vector<std::pair<std::string, std::string>> const &args) override {
    if (startParse) {
      auto &gateData = gates[instName];
      gateData.id = gnet.newGate();
      gateData.kind = symbol(moduleName);

      insertLink(args[0].second, instName, 0);
      for (size_t i = 1; i < args.size(); ++i) {
        insertLink(args[i].second, instName, 1);
      }
    }
  }

  /*! \brief Callback method for parsed endmodule.
   *
   */
  virtual void on_endmodule() override {
    if (startParse) {
      //  Collect links to make inputs arrays.
      for (auto &[name, links] : links) {
        auto source = gates.find(links[0]);
        auto target = gates.find(links[1]);

        if (source != gates.end() && target != gates.end()) {
          target->second.inputs.push_back(source->second.id);
        }
      }

      //  By that moment all gates are created - modifying them.
      for (const auto &[name, gateData] : gates) {
        std::vector<eda::gate::model::Signal> signals;
        signals.reserve(gateData.inputs.size());

        for (auto input : gateData.inputs) {
          signals.emplace_back(eda::rtl::model::Event::Kind::ALWAYS, input);
        }

        gnet.setGate(gateData.id, gateData.kind, signals);
      }
    }
  }

  void print() const {
    for (auto &gate : gnet.gates()) {
      std::cout << gate->id() << " " << gate->kind() << " :\n";
      for (auto &link : gate->links()) {
        std::cout << "\t( " << link.source << " ) " << link.target << "\n";
      }
    }
  }

  static void print(const eda::gate::model::Gate *gate) {
    std::cout << gate->kind() << gate->id();
  }

  void dot(std::ofstream& stream) const {
    stream << "digraph gnet {\n";
    for (const auto &gate : gnet.gates()) {
      for (auto &links : gate->links()) {
        stream << "\t";
        print(gate);
        stream << " -> ";
        print(gnet.gate(links.target));
        stream << ";\n";
      }
    }
    stream << "}" << std::endl;
  }

private:
  void insertLink(const std::string &name, const std::string &instName,
                  bool gate) {
    auto &link = links[name];
    if (link[gate] == "") {
      link[gate] = instName;
    } else {
      std::string nickname = name;
      while (links.find(nickname) != links.end()) {
        nickname += "#2";
      }
      auto &link2 = links[nickname] = link;
      link2[gate] = instName;
    }
  }

  static eda::gate::model::GateSymbol symbol(const std::string &s) {
    if (s == "not") {
      return eda::gate::model::GateSymbol::NOT;
    } else if (s == "or") {
      return eda::gate::model::GateSymbol::OR;
    } else if (s == "xor") {
      return eda::gate::model::GateSymbol::XOR;
    } else if (s == "nand") {
      return eda::gate::model::GateSymbol::NAND;
    } else if (s == "nor") {
      return eda::gate::model::GateSymbol::NOR;
    } else if (s == "xnor") {
      return eda::gate::model::GateSymbol::XNOR;
    }
    return eda::gate::model::GateSymbol::ZERO;
  }
};

