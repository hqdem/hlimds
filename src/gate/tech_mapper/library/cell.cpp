//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/tech_mapper/library/cell.h"

#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>

using json = nlohmann::json;

namespace eda::gate::optimizer {
  void LibraryCells::readLibertyFile(std::string filename) {

    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path PythonScriptPath = homePath / "src" / "gate" / "tech_mapper" / "library" / "libertyToJson.py";
    const std::filesystem::path outputPath = homePath / "test" / "data" / "gate" / "tech_mapper" / "liberty.json";

    std::string CallPythonParser = "python3 " + PythonScriptPath.string() + ' ' + filename  + ' ' + outputPath.string();
    
    std::cout << CallPythonParser << std::endl;

    system(CallPythonParser.c_str());

    // Open the JSON file and parse its contents
    std::ifstream ifs(outputPath.string());
    json j;
    ifs >> j;

    // Iterate over the objects in the JSON file
    for (json::iterator it = j.begin(); it != j.end(); ++it) {

      // Extract the truth table
      const std::string truthTableName = it.value()["output"].begin().key();
      const std::string plainTruthTable = it.value()["output"][truthTableName];

      // Create a dynamic truth table with the appropriate number of inputs
      std::vector<std::string> inputPinNames;

      const std::string inputs = it.value()["input"];
      std::string token;
      std::stringstream ss(inputs);
      while (getline(ss, token, ' ')) {
        inputPinNames.push_back(token);
      }

      kitty::dynamic_truth_table *truthTable =
          new kitty::dynamic_truth_table(inputPinNames.size());
      kitty::create_from_formula(*truthTable, plainTruthTable, inputPinNames);

      std::vector<Pin> pins;
    
      // Extract the input pins and their timing information
      for (const auto &name: inputPinNames) {
        const auto &cell = it.value()["delay"][name];
        pins.push_back(Pin(name, cell["cell_fall"], cell["cell_rise"],
          cell["fall_transition"], cell["rise_transition"]));
      }
     /*
      for (long unsigned int i = 0; i < inputPinNames.size(); ++i) {
        const std::string name = inputPinNames[i];

        double cell_fall= it.value()["delay"][name]["cell_fall"];
        double cell_rise = it.value()["delay"][name]["cell_rise"];
        double fall_transition = it.value()["delay"][name]["fall_transition"];
        double rise_transition = it.value()["delay"][name]["rise_transition"];

        pins.push_back(Pin(name, cell_fall, cell_rise,
            fall_transition, rise_transition));
      }
      */
      Cell *cell = new Cell(it.key(), pins, truthTable, it.value()["area"]);
      cells.push_back(cell);
    }
  }
} // namespace eda::gate::optimizer
