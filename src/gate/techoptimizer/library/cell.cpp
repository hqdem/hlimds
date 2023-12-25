//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/rwdatabase.h"
#include "gate/optimizer/visitor.h"
#include "gate/optimizer2/resynthesis/isop.h"
#include "gate/techoptimizer/library/cell.h"

#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

namespace eda::gate::tech_optimizer {

using RWDatabase = eda::gate::optimizer::RWDatabase;
using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;
using BoundGNet = eda::gate::optimizer::RWDatabase::BoundGNet;

using MinatoMorrealeAlg = eda::gate::optimizer2::resynthesis::MinatoMorrealeAlg;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using NetID = eda::gate::model::NetID;

//===----------------------------------------------------------------------===//
// Pin
//===----------------------------------------------------------------------===//

Pin::Pin(const std::string &name, double cell_fall, double cell_rise,
    double fall_transition, double rise_transition)
  : name(name), cell_fall(cell_fall), cell_rise(cell_rise),
    fall_transition(fall_transition), rise_transition(rise_transition) {}

const std::string& Pin::getName() const {
  return name;
}
double Pin::getMaxDelay() const {
  double riseDelay = cell_rise + rise_transition;
  double fallDelay = cell_fall + fall_transition;
  return (riseDelay >= fallDelay ? riseDelay : fallDelay);
}

//===----------------------------------------------------------------------===//
// Cell
//===----------------------------------------------------------------------===//

Cell::Cell(const std::string &name, const std::vector<Pin> &inputPins,
          kitty::dynamic_truth_table *truthTable, const std::string &realName, 
          double area)
  : name(name), inputPins(inputPins), truthTable(truthTable),
      realName(realName), area(area) {}

Cell::Cell(const std::string &name, const std::vector<Pin> &inputPins,
          kitty::dynamic_truth_table *truthTable, const std::string &realName)
  : name(name), inputPins(inputPins), truthTable(truthTable),
      realName(realName), area(0.0) {}

Cell::Cell(kitty::dynamic_truth_table *truthTable) :
  name(""), inputPins({}), truthTable(truthTable) {}

const std::string &Cell::getName() const {return name;}
double Cell::getArea() const {return area;}
kitty::dynamic_truth_table *Cell::getTruthTable() const {return truthTable;}
unsigned Cell::getInputPinsNumber() const {return inputPins.size();}
const Pin &Cell::getInputPin(uint inputPinNumber) const {
    assert(inputPinNumber < inputPins.size());
    return inputPins[inputPinNumber];
}

//===----------------------------------------------------------------------===//
// LibraryCells
//===----------------------------------------------------------------------===//

LibraryCells::LibraryCells(const std::string &filename) {
  readLibertyFile(filename);
}

void LibraryCells::readLibertyFile(const std::string &filename) {

  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path PythonScriptPath = homePath / "src" / "gate" / "techoptimizer" / "library" / "libertyToJson.py";
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

      Cell *cell = new Cell(it.key() + std::to_string(i), 
          pins, truthTable, it.key() + std::string(""), it.value()["area"]);
      cells.push_back(cell);
    } while(next_permutation(inputPinNames.begin(), inputPinNames.end()));
  }
}

void LibraryCells::initializeLibraryRwDatabase(SQLiteRWDatabase *arwdb,
    std::unordered_map<std::string, CellTypeID> &cellTypeMap) {
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
    Gate::Id outputId;

    model::GateSymbol customName =
        model::GateSymbol::create(cell->getName());

    auto cellNet = std::make_shared<GNet>();
    for (unsigned i = 0; i < cell->getInputPinsNumber(); i++) {
      const Gate::Id inputId = cellNet->addIn();
      inputs.push_back(Gate::Signal::always(inputId));
    }

    auto gateId = cellNet->addGate(customName, inputs);
    outputId = cellNet->addOut(gateId);

    cellNet->sortTopologically();
    
    std::shared_ptr<GNet> dummy = cellNet;

    BoundGNet::GateBindings bindings;
    std::vector<double> delay;

    for (unsigned int i = 0; i < cell->getInputPinsNumber(); i++) {
        bindings.push_back(inputs[i].node());

      
        Pin pin = cell->getInputPin(i);
        double pinDelay = pin.getMaxDelay();
        delay.push_back(pinDelay);
      }

    dummy->sortTopologically();

    BoundGNet::BoundGNetList bgl;
    BoundGNet bg {dummy, bindings, {outputId}, delay, cell->getName(), cell->getArea()};
    bgl.push_back(bg);

    RWDatabase::TruthTable TT(truthTable);

    auto list = arwdb->get(truthTable);
    if (list.size() == 0) {
       arwdb->set(TT, bgl);
    } else {
      list.push_back(bg);
      arwdb->set(TT, list);
    }

    eda::gate::model::CellProperties props{0, 0, 0, 0, 0, 0, 0};
    CellTypeID cellID = eda::gate::model::makeCellType(
        cell->getName(), eda::gate::model::CellSymbol::CELL,
        props, static_cast<uint16_t>(cell->getInputPinsNumber()), 
        static_cast<uint16_t>(1));
    cellTypeMap.insert(std::pair<std::string, CellTypeID>
          (cell->getName(), cellID));
  }
  }

  std::list<CellTypeID> LibraryCells::initializeLiberty() {
    std::list<CellTypeID> cellTypeIDs;

    for(auto& cell : cells) {

      if (cell->getInputPinsNumber() == 0 ) {
        continue;
      }

      eda::gate::model::CellProperties props(true, false, false, false, false, false, false);
      eda::gate::model::CellTypeAttrID attrID;

      MinatoMorrealeAlg minatoMorrealeAlg;
      const auto subnetID = minatoMorrealeAlg.synthesize(*cell->getTruthTable());

      NetID netID = static_cast<NetID>(subnetID);

      CellTypeID cellID = eda::gate::model::makeCellType(
          cell->getName(), netID, attrID,
          eda::gate::model::CellSymbol::CELL,
          props, static_cast<uint16_t>(cell->getInputPinsNumber()), 
          static_cast<uint16_t>(1));

      cellTypeIDs.push_back(cellID);
    }
    return cellTypeIDs;
  }


} // namespace eda::gate::tech_optimizer
