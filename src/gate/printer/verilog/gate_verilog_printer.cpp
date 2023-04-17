//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate_verilog_printer.h"
#include "util/assert.h"

#include "ctemplate/template.h"
#include <chrono>
#include <ctime>
#include <vector>

namespace eda::gate::printer {

void GateVerilogPrinter::print(std::ostream &out, const GNet &net) const {
  using time = std::chrono::system_clock;
  if (net.isWellFormed()) {
    std::string netName = NET_NAME_PREFIX + std::to_string(net.id());
    ctemplate::TemplateDictionary dictionary(DICTIONARY_NAME);
    const auto now = time::to_time_t(time::now());
    std::string dateTime(std::ctime(&now));
    dictionary.SetValue(GEN_TIME, dateTime);
    // Print top module
    auto *moduleDict = dictionary.AddSectionDictionary(MODULES);
    moduleDict->SetValue(MODULE_NAME, netName);

    std::vector<std::string> wires;
    std::vector<const Gate*> moduleInputs;
    std::vector<const Gate*> moduleOutputs;

    for (const auto *gate : net.gates()) {
      if (gate->isSource()) {
        moduleInputs.push_back(gate);
      } else if (gate->isTarget()) {
        moduleOutputs.push_back(gate);
      } else if (gate->isValue()) {
        // TODO
      } else {
        // Print gate
        const auto &gateInputs = gate->inputs();
        const auto size = gateInputs.size();
        // Check if we need a stub
        /*if (size != 2) {
          // Print stub
          auto *stubDict = dictionary.AddSectionDictionary(MODULES);
          stubDict->SetValue(MODULE_NAME, netName);
          auto *outDict = stubDict->AddSectionDictionary(OUTS);
          outDict->SetValue(OUTPUT, "out");
          outDict->SetValue(SEPARATOR, ",");
          for (unsigned int i = 1; i <= size; i++) {
            const std::string separator = (i < size) ? "," : "";
            auto *inDict = moduleDict->AddSectionDictionary(INS);
            inDict->SetValue(INPUT, "in" + std::to_string(i));
            inDict->SetValue(SEPARATOR, separator);
          }
        }*/
        const auto id = gate->id();
        const auto gateType = gate->func().name();
        const auto gateOutName = wire(id);
        auto *gateDict = moduleDict->AddSectionDictionary(GATES);
        gateDict->SetValue(GATE_TYPE, gate->func().name());
        gateDict->SetValue(GATE_NAME, GATE_NAME_PREFIX + std::to_string(id));
        gateDict->SetValue(GATE_OUT, gateOutName);
        wires.push_back(gateOutName);
        
        // Print gate inputs
        for (unsigned int i = 0; i < size; i++) {
          //const std::string separator = (i < size - 1) ? "," : "";
          auto *portDictionary = moduleDict->AddSectionDictionary(G_INS);
          portDictionary->SetValue(GATE_IN, wire(gateInputs.at(i).node()));
          //portDictionary->SetValue(SEPARATOR, separator);
        }
      }
    }

    // Print wires
    for (const auto &wireName : wires) {
      moduleDict->AddSectionDictionary(WIRES)->SetValue(WIRE_NAME, wireName);
    }

    // Print module inputs
    const auto numInputs = moduleInputs.size();
    for (unsigned i = 0; i < numInputs; i++) {
      const std::string separator = (i != numInputs - 1) ? "," : "";
      auto *gateDict = moduleDict->AddSectionDictionary(INS);
      gateDict->SetValue(INPUT, wire(moduleInputs.at(i)->id()));
      gateDict->SetValue(SEPARATOR, separator);
    }

    // Print module outputs
    const auto numOutputs = moduleOutputs.size();
    for (unsigned i = 0; i < numOutputs; i++) {
      const auto *output = moduleOutputs.at(i);
      const auto outputName = wire(output->id());
      const std::string separator = 
        (i != numOutputs - 1) ? "," : (numInputs != 0) ? "," : "";

      auto *gateDict = moduleDict->AddSectionDictionary(OUTS);
      gateDict->SetValue(OUTPUT, outputName);
      gateDict->SetValue(SEPARATOR, separator);
      
      uassert(output->arity() == 1, "Arity of the output is expected to be 1!");
      
      // Print output assignment
      const auto driverName = wire(output->input(0).node());
      auto *assignDict = moduleDict->AddSectionDictionary(ASSIGNS);
      assignDict->SetValue(LHS, outputName);
      assignDict->SetValue(RHS, driverName);
    }

    std::string buffer;
    ctemplate::ExpandTemplate(templatePath, ctemplate::DO_NOT_STRIP, 
      &dictionary, &buffer);
    out << buffer;
  } else {
    // TODO
  }
}

} // namespace eda::gate::printer
