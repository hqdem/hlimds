//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/rwdatabase.h"
#include "gate/optimizer/visitor.h"
#include "gate/optimizer2/synthesis/isop.h"
#include "gate/techmapper/library/cell.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

//#include <list>

using json = nlohmann::json;
using std::filesystem::exists;
using std::filesystem::path;
using std::getline;
using std::ifstream;

namespace eda::gate::tech_optimizer {

using RWDatabase = eda::gate::optimizer::RWDatabase;
using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;
using BoundGNet = eda::gate::optimizer::RWDatabase::BoundGNet;

using MinatoMorrealeAlg = eda::gate::optimizer2::synthesis::MMSynthesizer;
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
          kitty::dynamic_truth_table *truthTable,
          double area)
  : name(name), inputPins(inputPins), truthTable(truthTable),
  area(area) {}

Cell::Cell(const std::string &name, const std::vector<Pin> &inputPins,
          kitty::dynamic_truth_table *truthTable)
  : name(name), inputPins(inputPins), truthTable(truthTable),
  area(0.0) {}

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

//LibraryCells::LibraryCells(const std::string &filename) {
//  readLibertyFile(filename);
//}
/*
void LibraryCells::readLibertyFile(const std::string &filename, std::vector<Cell*> &cells) {

  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path PythonScriptPath = homePath / "src" / "gate" / "techmapper" / "library" / "libertyToJson.py";
  const std::filesystem::path outputPath = homePath / "test" / "data" / "gate" / "techmapper" / "liberty.json";

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

    std::vector<Pin> pins;
    for (const auto &name: inputPinNames) {
      const auto &cell = it.value()["delay"][name];
      pins.push_back(Pin(name, cell["cell_fall"], cell["cell_rise"],
        cell["fall_transition"], cell["rise_transition"]));
    }

    kitty::dynamic_truth_table *truthTable =
      new kitty::dynamic_truth_table(inputPinNames.size());
    kitty::create_from_formula(*truthTable, plainTruthTable, inputPinNames);

    Cell *cell = new Cell(it.key(), pins, truthTable, it.value()["area"]);
    cells.push_back(cell);
  }
}*/

void LibraryCells::readLibertyFile(const std::string &filename,
                                   std::vector<CellTypeID> &cellTypeIDs,
                                   std::vector<CellTypeID> &cellTypeFFIDs,
                                   std::vector<CellTypeID> &cellTypeFFrsIDs,
                                   std::vector<CellTypeID> &cellTypeLatchIDs) {
/*
  TokenParser tokParser;
  FILE *file = fopen(filename.c_str(), "rb");
  Group *ast = tokParser.parseLibrary(file,
                                      filename.c_str());
  Library lib;
  AstParser parser(lib, tokParser);
  parser.run(*ast);
  fclose(file);

  for (const auto& cell : lib.getCells()) {
    auto name = std::string(cell.getName());

    std::vector<std::string> inputs;
    std::vector<std::string> outputs;

    std::vector<std::string> funcs;

    for (const auto &pin : cell.getPins()) {
      if (pin.getIntegerAttribute("direction", 10) & (1 << 0)) {
        inputs.push_back(std::string(pin.getName()));
      }
      if (pin.getIntegerAttribute("direction", 10) & (1 << 1)) {
          outputs.push_back(std::string(pin.getName()));
        if (pin.hasAttribute("function")) {
          funcs.push_back(std::string(
              pin.getStringAttribute("function" , "none")));
        }
      }
    }

    eda::gate::model::CellProperties
        props(false, false, false, false, false, false, false, false, false);

    model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
    model::CellTypeAttr::get(cellTypeAttrID).props.area =
        cell.getIntegerAttribute("area", 0);

    kitty::dynamic_truth_table *truthTable =
        new kitty::dynamic_truth_table(inputs.size());
    kitty::create_from_formula(*truthTable, funcs.at(0), inputs);

    MinatoMorrealeAlg minatoMorrealeAlg;
    const auto subnetID = minatoMorrealeAlg.synthesize(*truthTable);

    CellTypeID cellID = eda::gate::model::makeCellType(
        eda::gate::model::CellSymbol::UNDEF,
        name, subnetID,
        cellTypeAttrID, props,
        static_cast<uint16_t>(inputs.size()),
        static_cast<uint16_t>(1));

    cellTypeIDs.push_back(cellID);

  }*/

/*
  const path homePath1 = std::string(getenv("UTOPIA_HOME"));
  const path filePath = homePath1 / filename;
  if (exists(filePath)) {
    TokenParser tokParser;
    FILE *file = fopen(filePath.generic_string().c_str(), "rb");
    Group *ast = tokParser.parseLibrary(file,
                                        filePath.generic_string().c_str());
    Library lib;
    AstParser parser(lib, tokParser);
    parser.run(*ast);
    fclose(file);

    for (const auto &cell: lib.getCells()) {
      auto name = cell.getName();
      auto pins = cell.getPins();

      int nIn = 0;
      std::vector<std::string> inputPinNames;
      for (const auto &pin: pins) {
        //if (pin)
        //inputPinNames.push_back(pin.)
      }
      eda::gate::model::CellProperties
          props(true, false, false, false, false, false, false);

      model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
      model::CellTypeAttr::get(cellTypeAttrID).area = cell.getIntegerAttribute("area", 0);

      auto func = cell.getStringAttribute("function", "");
      kitty::dynamic_truth_table *truthTable =
          new kitty::dynamic_truth_table(nIn);
      kitty::create_from_formula(*truthTable, func, inputPinNames);

      MinatoMorrealeAlg minatoMorrealeAlg;
      const auto subnetID = minatoMorrealeAlg.synthesize(*truthTable);

      CellTypeID cellID = eda::gate::model::makeCellType(
          cell.getName(), subnetID, cellTypeAttrID,
          eda::gate::model::CellSymbol::CELL,
          props, static_cast<uint16_t>(2),
          static_cast<uint16_t>(1));

      cellTypeIDs.push_back(cellID);
    }
  } else {
    std::cerr << "File wasn't found\n";
  }*/

  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path PythonScriptPath =
      homePath / "src" / "gate" / "techmapper" / "library" /
      "libertyToJson.py";
  const std::filesystem::path outputPath =
      homePath / "test" / "data" / "gate" / "techmapper" / "liberty.json";

  std::string CallPythonParser =
      "python3 " + PythonScriptPath.string() + ' ' + filename + ' ' +
      outputPath.string();

  std::cout << CallPythonParser << std::endl;

  system(CallPythonParser.c_str());

  // Open the JSON file and parse its contents
  std::ifstream ifs(outputPath.string());
  json j;
  ifs >> j;

  // Iterate over the objects in the JSON file
  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    std::vector<std::string> inputPinNames;

    const std::string inputs = it.value()["input"];
    std::string token;
    std::stringstream ss(inputs);
    while (getline(ss, token, ' ')) {
      inputPinNames.push_back(token);
    }
    if (!inputPinNames.empty() ) {
      if (it.value().contains("comb")  &&
          it.value()["comb"].get<bool>() &&
          it.value().contains("output") &&
          it.value()["output"].size() == 1) {
        const std::string truthTableName = it.value()["output"].begin().key();
        const std::string plainTruthTable = it.value()["output"][truthTableName];

        // Create a dynamic truth table with the appropriate number of inputs

        *//*std::vector<Pin> pins;
        for (const auto &name: inputPinNames) {
          const auto &cell = it.value()["delay"][name];
          pins.push_back(Pin(name, cell["cell_fall"], cell["cell_rise"],
                             cell["fall_transition"], cell["rise_transition"]));
        }*//*

        kitty::dynamic_truth_table *truthTable =
            new kitty::dynamic_truth_table(inputPinNames.size());
        kitty::create_from_formula(*truthTable, plainTruthTable, inputPinNames);

        eda::gate::model::CellProperties
            props(true, true, true, false, false, false, false, false, false);

        model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
        model::CellTypeAttr::get(cellTypeAttrID).props.area = it.value()["area"];

        MinatoMorrealeAlg minatoMorrealeAlg;
        const auto subnetID = minatoMorrealeAlg.synthesize(*truthTable);

        CellTypeID cellID = eda::gate::model::makeCellType(
            eda::gate::model::CellSymbol::UNDEF,
            it.key(), subnetID,
            cellTypeAttrID, props,
            static_cast<uint16_t>(inputPinNames.size()),
            static_cast<uint16_t>(1));

        cellTypeIDs.push_back(cellID);

      } else {
        if (it.value().contains("ff") && it.value()["ff"].get<bool>()) {
          if (inputPinNames.size() == 2) {
            eda::gate::model::CellProperties
                props(false, false, false, false, false, false, false, false,
                      false);

            model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
            model::CellTypeAttr::get(cellTypeAttrID).props.area = it.value()["area"];

            SubnetBuilder subnetBuilder;
            auto in1 = subnetBuilder.addInput();
            auto in2 = subnetBuilder.addInput();
            auto dff = subnetBuilder.addCell(model::CellSymbol::DFF, in2, in1);
            subnetBuilder.addOutput(dff);

            CellTypeID cellID = eda::gate::model::makeCellType(
                eda::gate::model::CellSymbol::UNDEF,
                it.key(), subnetBuilder.make(),
                cellTypeAttrID, props,
                static_cast<uint16_t>(inputPinNames.size()),
                static_cast<uint16_t>(it.value()["output"].size()));

            cellTypeFFIDs.push_back(cellID);
          }
        } else if (it.value().contains("ffrs") && it.value()["ffrs"].get<bool>()) {
          if (inputPinNames.size() == 4) {
            eda::gate::model::CellProperties
                props(false, false, false, false, false, false, false, false,
                      false);

            model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
            model::CellTypeAttr::get(cellTypeAttrID).props.area = it.value()["area"];

            SubnetBuilder subnetBuilder;
            auto in1 = subnetBuilder.addInput();
            auto in2 = subnetBuilder.addInput();
            auto in3 = subnetBuilder.addInput();
            auto in4 = subnetBuilder.addInput();
            auto dffrs = subnetBuilder.addCell(model::CellSymbol::DFFrs, in2, in1, in3, in4);
            subnetBuilder.addOutput(dffrs);

            CellTypeID cellID = eda::gate::model::makeCellType(
                eda::gate::model::CellSymbol::UNDEF,
                it.key(), subnetBuilder.make(),
                cellTypeAttrID, props,
                static_cast<uint16_t>(inputPinNames.size()),
                static_cast<uint16_t>(it.value()["output"].size()));

            cellTypeFFrsIDs.push_back(cellID);
          }

        } else if (it.value().contains("latch") && it.value()["latch"].get<bool>()) {
          if (inputPinNames.size() == 2) {
            eda::gate::model::CellProperties
                props(false, false, false, false, false, false, false, false,
                      false);

            model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
            model::CellTypeAttr::get(cellTypeAttrID).props.area = it.value()["area"];

            SubnetBuilder subnetBuilder;
            auto in1 = subnetBuilder.addInput();
            auto in2 = subnetBuilder.addInput();
            auto latch = subnetBuilder.addCell(model::CellSymbol::LATCH, in1, in2);
            subnetBuilder.addOutput(latch);

            CellTypeID cellID = eda::gate::model::makeCellType(
                eda::gate::model::CellSymbol::UNDEF,
                it.key(), subnetBuilder.make(),
                cellTypeAttrID, props,
                static_cast<uint16_t>(inputPinNames.size()),
                static_cast<uint16_t>(it.value()["output"].size()));

            cellTypeLatchIDs.push_back(cellID);
          }
        }
      }
    }
  }
}

/*void LibraryCells::makeCellTypeIDs(std::vector<Cell*> &cells, std::vector<CellTypeID> &cellTypeIDs) {
  for(auto& cell : cells) {
    if (cell->getInputPinsNumber() == 0 ) {
      continue;
    }

    eda::gate::model::CellProperties
      props(true, true, false, false, false, false, false, false, false);

    model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
    model::CellTypeAttr::get(cellTypeAttrID).props.area = cell->getArea();

    MinatoMorrealeAlg minatoMorrealeAlg;
    const auto subnetID = minatoMorrealeAlg.synthesize(*cell->getTruthTable());

    CellTypeID cellID = eda::gate::model::makeCellType(
      cell->getName(), subnetID, cellTypeAttrID,
      eda::gate::model::CellSymbol::CELL,
      props, static_cast<uint16_t>(cell->getInputPinsNumber()),
      static_cast<uint16_t>(1));

    cellTypeIDs.push_back(cellID);
  }

  for (Cell* ptr : cells) {
    delete ptr;
  }
  cells.clear();
}*/

} // namespace eda::gate::tech_optimizer
