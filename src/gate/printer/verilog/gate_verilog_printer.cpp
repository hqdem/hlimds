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
#include <vector>

namespace eda::gate::printer {

void GateVerilogPrinter::print(std::ostream &out, const GNet &net) const {
  if (net.isWellFormed()) {
    std::string netName = NET_NAME_PREFIX + std::to_string(net.id());
    ctemplate::TemplateDictionary dictionary(DICTIONARY_NAME);
    dictionary.SetValue(GEN_TIME, "");
    dictionary.SetValue(MODULE_NAME, netName);

    std::vector<std::string> wires;
    std::vector<const Gate*> inputs;
    std::vector<const Gate*> outputs;

    for (const auto *gate : net.gates()) {
      if (gate->isSource()) {
        //auto *gateDict = dictionary.AddSectionDictionary(INS);
        inputs.push_back(gate);
      } else if (gate->isTarget()) {
        outputs.push_back(gate);
      } else if (gate->isValue()) {
        // TODO
      } else {
        auto *gateDict = dictionary.AddSectionDictionary(GATES);
        const auto gateType = gate->func().name();
        const auto gateId = std::to_string(gate->id());
        const auto gateName = GATE_NAME_PREFIX + gateId;
        const auto gateOutName = wire(gate->id());
        gateDict->SetValue(GATE_TYPE, gateType);
        gateDict->SetValue(GATE_NAME, gateName);
        gateDict->SetValue(GATE_OUT, gateOutName);
        wires.push_back(gateOutName);

        unsigned int portCount = 0;
        const auto &inputs = gate->inputs();
        for (const auto &signal : inputs) {
          auto *portDictionary = dictionary.AddSectionDictionary(G_INS);
          const auto name = wire(signal.node());
          const std::string separator = 
            (portCount < inputs.size() - 1) ? "," : "";
          portDictionary->SetValue(GATE_IN, name);
          portDictionary->SetValue(SEPARATOR, separator);
          portCount++;
        }
      }
    }

    for (const auto &wireName : wires) {
      auto *wireDictionary = dictionary.AddSectionDictionary(WIRES);
      wireDictionary->SetValue(WIRE_NAME, wireName);
    }

    unsigned int inputCount = 0;
    for (const auto *input : inputs) {
      const auto inputName = wire(input->id());
      const std::string separator = (inputCount < inputs.size() - 1) ? "," : 
        (outputs.size() != 0) ? "," : "";
      inputCount++;

      auto *gateDict = dictionary.AddSectionDictionary(INS);
      gateDict->SetValue(INPUT, inputName);
      gateDict->SetValue(SEPARATOR, separator);
    }

    unsigned int outputCount = 0;
    for (const auto *output : outputs) {
      const auto outputName = wire(output->id());
      const std::string separator = 
        (outputCount < outputs.size() - 1) ? "," : "";
      outputCount++;
      auto *gateDict = dictionary.AddSectionDictionary(OUTS);
      gateDict->SetValue(OUTPUT, outputName);
      gateDict->SetValue(SEPARATOR, separator);
      
      // TODO: check this is true
      uassert(output->arity() == 1, "Arity of the output is expected to be 1!");
      const auto driverName = wire(output->input(0).node());
      auto *assignDict = dictionary.AddSectionDictionary(ASSIGNS);
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
