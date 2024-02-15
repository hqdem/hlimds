//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/miter.h"
#include "util/assert.h"
#include "vcd.h"

#include <chrono>
#include <ctemplate/template.h>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <vector>

namespace eda::gate::printer {

void VCDPrinter::print(std::ostream &out,
                       const GNet &net,
                       const std::vector<bool> &values) const {
  using time = std::chrono::system_clock;
  std::string netName = NET_NAME_PREFIX + std::to_string(net.id());
  ctemplate::TemplateDictionary dictionary(DICTIONARY_NAME);
  const auto now = time::to_time_t(time::now());
  std::string dateTime(std::ctime(&now));
  dictionary.SetValue(GEN_TIME, dateTime);
  dictionary.SetValue(NET_ID, netName);
  const auto &gates = net.gates();

  // Print vars
  for (size_t i = 0; i < gates.size(); i++) {
    auto *varDict = dictionary.AddSectionDictionary(VARS);
    auto gid = gates[i]->id();
    varDict->SetValue(VAR_ID, VAR + std::to_string((gid)));
    varDict->SetValue(GID, GATE_NAME_PREFIX + std::to_string(gid));
  }

  debugger::Compiled compiled = debugger::makeCompiled(net);
  std::vector<bool> outs(net.nOuts());
  compiled.simulate(outs, values);

  // Print values
  for (size_t i = 0; i < gates.size(); i++) {
    auto *valDict = dictionary.AddSectionDictionary(VALUES);
    auto gid = gates[i]->id();
    valDict->SetValue(VALUE, std::to_string(compiled.getValue(gid)));
    valDict->SetValue(VAR_ID, VAR + std::to_string(gid));
  }

  std::string buffer;
  const char* path = getenv("UTOPIA_HOME");
  if (path != nullptr) {
    const std::filesystem::path homePath = std::string(path);
    std::filesystem::path templatePath = homePath / TEMPLATE_PATH;
    ctemplate::ExpandTemplate(templatePath.string(),
                              ctemplate::DO_NOT_STRIP,
                              &dictionary, &buffer);
    out << buffer;
  }
}

void VCDPrinter::print(const std::string &filename,
                       const GNet &net,
                       const std::vector<bool> &values) const {
  std::ofstream outFile(filename);
  uassert(outFile.is_open(),
          "Could not create a file: " + filename + '\n');
  if (outFile.is_open()) {
    print(outFile, net, values);
  }
}

void VCDPrinter::print(const std::string &filename,
                       const GNet &net,
                       const CheckerResult res) const {
  print(filename, net, res.getCounterExample());
}

} // namespace eda::gate::printer
