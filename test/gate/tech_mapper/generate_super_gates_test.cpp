#include "gate/tech_mapper/library/cell.h"
#include "gate/tech_mapper/super_gate_generator/generateBestCircuits.h"

#include "gtest/gtest.h"
#include <filesystem>

const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
const std::filesystem::path libertyDirrect = homePath / "test" / "data" / "gate" / "tech_mapper";

using namespace eda::gate::optimizer;

bool checkingGenerationBestCircuits(std::string liberty) { 
  const std::string pathToLiberty = libertyDirrect / liberty;
  LibraryCells libraryCells(pathToLiberty);

  CircuitsGenerator bestCircuitsGenerator;

  bestCircuitsGenerator.setLibElementsList(libraryCells.cells);

  // Прописываем кол-во входов схемы
  bestCircuitsGenerator.initCircuit(4);

  // Генерируем схемы
  bestCircuitsGenerator.generateCircuits();

  // Выводим показатели 
  for (const auto& node : bestCircuitsGenerator.getGeneratedNodes()) {
    kitty::print_binary(*(node->getCell()->getTruthTable()), std::cout);
    std::cout << '\n';
  }

  std::cout << "\nThe number of nodes: " <<
	  bestCircuitsGenerator.getGeneratedNodes().size() << "\n";

  return true;
}

TEST(GenerateSuperGatesTest, sky130_fd_sc_hd__ff_100C_1v65) {
  if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

  checkingGenerationBestCircuits(libertyDirrect.string() + "/sky130_fd_sc_hd__ff_100C_1v65.lib");
}