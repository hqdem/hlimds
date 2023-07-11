//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/tech_mapper/library/cell.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/optimizer/visitor.h"

#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>

using json = nlohmann::json;

namespace eda::gate::techMap {
  using RWDatabase = eda::gate::optimizer::RWDatabase;
  using Gate = eda::gate::model::Gate;
  using GNet = eda::gate::model::GNet;
  using BoundGNet = eda::gate::optimizer::RWDatabase::BoundGNet;
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
      int i = 0;
      do {
        i++;
        std::vector<Pin> pins;
        for (const auto &name: inputPinNames) {
          const auto &cell = it.value()["delay"][name];
          pins.push_back(Pin(name, cell["cell_fall"], cell["cell_rise"],
            cell["fall_transition"], cell["rise_transition"]));
        }

        kitty::dynamic_truth_table *truthTable =
          new kitty::dynamic_truth_table(inputPinNames.size());
        kitty::create_from_formula(*truthTable, plainTruthTable, inputPinNames);

        Cell *cell = new Cell(it.key() + std::to_string(i), pins, truthTable, it.value()["area"]);
        cells.push_back(cell);
      } while(next_permutation(inputPinNames.begin(), inputPinNames.end()));
    }
  }

  void LibraryCells::initializeLibraryRwDatabase(SQLiteRWDatabase *arwdb) {
  for(auto& cell : cells) {
    uint64_t truthTable = 0;

    if (cell->getInputPinsNumber() == 0 ) {
      continue;
    }

    truthTable = 0;
    uint64_t _1 = 1;
    for (uint64_t i = 0; i < 64; i++) {
      if (kitty::get_bit(*(cell->getTruthTable()), 
          i % cell->getTruthTable()->num_bits())) {
        truthTable |= (_1 << i);
      }
    }

    Gate::SignalList inputs;

    model::GateSymbol customName =
        model::GateSymbol::create(cell->getName());

    auto cellNet = std::make_shared<GNet>();
    for (unsigned i = 0; i < cell->getInputPinsNumber(); i++) {
      const Gate::Id inputId = cellNet->addIn();
      inputs.push_back(Gate::Signal::always(inputId));
    }

    cellNet->addGate(customName, inputs);

    cellNet->sortTopologically();
    
    std::shared_ptr<GNet> dummy = cellNet;

    BoundGNet::GateBindings bindings;
    std::vector<double> delay;

    for (unsigned int i = 0; i < cell->getInputPinsNumber(); i++) {
        bindings.push_back(inputs[i].node());

      
        Pin pin = cell->getInputPin(i);
        double pinDelay = pin.getMaxdelay();
        delay.push_back(pinDelay);
      }

    dummy->sortTopologically();

    BoundGNet::BoundGNetList bgl;
    BoundGNet bg {dummy, bindings, {}, delay, cell->getName(), cell->getArea()};
    bgl.push_back(bg);

    RWDatabase::TruthTable TT(truthTable);

    auto list = arwdb->get(truthTable);
    if (list.size() == 0) {
       arwdb->set(TT, bgl);
    } else {
      list.push_back(bg);
      arwdb->set(TT, list);
    }
  }
}
} // namespace eda::gate::techMap
