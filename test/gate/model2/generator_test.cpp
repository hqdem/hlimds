//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#define FANOUTNOTCHECK

#include "gate/model2/generator/matrix_generator.h"
#include "gate/model2/generator/layer_generator.h"
#include "gate/model2/printer/dot.h"
#include "gate/optimizer/optimizer_util.h"

#include "gtest/gtest.h"

#include <fstream>

namespace eda::gate::model {

const std::string genTestFolder = "/output/test/generator/";

bool checkCellsValid(const List<CellID> &cells) {
  for (const auto &cellId : cells) {
    const Cell &cell = Cell::get(cellId);
    const CellType &cellType = cell.getType();
    const uint16_t cellFanin = cell.getFanin();
#ifndef FANOUTNOTCHECK
    const uint16_t cellFanout = cell.getFanout();
    const CellSymbol &cellSymbol = cell.getType().getSymbol();
#endif
    if ((cellType.isAnyArity() && cellFanin < 2) &&
        (cellType.getInNum() != cellFanin)) {

      return false;
    }
#ifndef FANOUTNOTCHECK
    if ((cellSymbol == OUT && cellFanout > 0) ||
        (cellSymbol != OUT && cellFanout == 0)) {
      return false;
    }
#endif
  }
  return true;
}

bool netValid(const Net &net, const int nCells,
              const int nIn, const int nOut,
              const bool generatable) {

  const uint16_t netInN = net.getInNum(), netOutN = net.getOutNum();
  const uint32_t netCombN = net.getCombNum();
  if (!generatable) {
    return !netInN && !netOutN && !netCombN;
  }
  if (!(netInN == nIn && netOutN == nOut &&
        ((int)netCombN + (int)net.getFlipNum()) +
        (int)net.getSoftNum() + (int)net.getHardNum() == nCells)) {

    return false;
  }

  return checkCellsValid(net.getCombCells()) &&
         checkCellsValid(net.getFlipFlops()) &&
         checkCellsValid(net.getOutputs()) &&
         checkCellsValid(net.getInputs());
}

void printGeneratedNet(const Net &net, const std::string &subFolder,
                       const std::string &fileName) {
  NetPrinter *dotPrinter = &NetPrinter::getDefaultPrinter();
  std::ofstream out;
  std::string homePath = getenv("UTOPIA_HOME");
  std::filesystem::path filePath = optimizer::createOutPath(homePath +
                                                            genTestFolder +
                                                            subFolder);
  out.open(filePath.c_str() + fileName);
  dotPrinter->print(out, net);
  out.close();
}

void setFaninLim(Generator *generator,
                 const std::pair<uint16_t, uint16_t> *faninLim) {

  if (generator == nullptr) {
    return;
  }
  if (faninLim != nullptr) {
    if (!faninLim->first) {
      generator->setFaninHigh(faninLim->second);
    } else {
      generator->setFaninLim(faninLim->first, faninLim->second);
    }
  }
}

CellTypeID createNetCell() {
  NetBuilder netBuilder;
  auto cellINID = makeCell(IN);
  auto cellINID2 = makeCell(IN);
  auto cellANDID = makeCell(AND, { LinkEnd(cellINID), LinkEnd(cellINID2) });
  auto cellOUTID = makeCell(OUT, { LinkEnd(cellANDID) });

  netBuilder.addCell(cellINID);
  netBuilder.addCell(cellINID2);
  netBuilder.addCell(cellANDID);
  netBuilder.addCell(cellOUTID);

  return makeCellType("net", netBuilder.make(), OBJ_NULL_ID, NET,
                      CellProperties(1, 0, 0, 0, 0), 2, 1);
}

template<typename BaseT>
void startMatrixGenerator(const int nCells, const int nIn,
                          const int nOut,
                          const std::initializer_list<BaseT> &netBase,
                          const std::string &fileName,
                          const bool generatable,
                          const std::pair<uint16_t, uint16_t> *faninLim,
                          const bool hierarchical,
                          const unsigned seed = 0u) {

  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }

  auto generator = seed ? MatrixGenerator(nCells, nIn, nOut, netBase, seed) :
                          MatrixGenerator(nCells, nIn, nOut, netBase);
  setFaninLim(&generator, faninLim);
  generator.setHierarchical(hierarchical);
  Net &net = Net::get(generator.generate());
  EXPECT_TRUE(netValid(net, nCells, nIn, nOut, generatable));
  printGeneratedNet(net, "matrix/", fileName);
}

template<typename BaseT>
void startLayerGenerator(const int nIn, const int nOut,
                         const std::initializer_list<BaseT> &netBase,
                         const std::vector<int> &layerNCells,
                         const std::string &fileName,
                         const bool generatable,
                         const std::pair<uint16_t, uint16_t> *faninLim,
                         const bool hierarchical,
                         const unsigned seed = 0u) {

  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }

  auto generator = seed ?
                   LayerGenerator(nIn, nOut, netBase, layerNCells, seed) :
                   LayerGenerator(nIn, nOut, netBase, layerNCells);
  setFaninLim(&generator, faninLim);
  generator.setHierarchical(hierarchical);
  Net &net = Net::get(generator.generate());

  int nCells = 0;
  for (const int nCellsOnLayer: layerNCells) {
    nCells += nCellsOnLayer;
  }
  EXPECT_TRUE(netValid(net, nCells, nIn, nOut, generatable));
  printGeneratedNet(net, "layers/", fileName);
}

// Matrix generator tests.

TEST(MatrixMatrixGeneratorTest, MinCells) {
  startMatrixGenerator(0, 1, 1, { AND, NOT }, "min_cells.dot", true, nullptr,
                       false);
}

TEST(MatrixGeneratorTest, OnlyNot) {
  startMatrixGenerator(1, 1, 1, { NOT }, "only_not.dot", true, nullptr, false);
}

TEST(MatrixGeneratorTest, SeveralIn) {
  startMatrixGenerator(40, 30, 1, { AND, NOT }, "several_in.dot", true,
                       nullptr, false);
}

TEST(MatrixGeneratorTest, SeveralOut) {
  startMatrixGenerator(40, 1, 30, { AND, NOT }, "several_out.dot", true,
                       nullptr, false);
}

TEST(MatrixGeneratorTest, SeveralInOut) {
  startMatrixGenerator(40, 10, 50, { AND, NOT }, "several_in_out.dot", true,
                       nullptr, false);
}

TEST(MatrixGeneratorTest, Ungeneratable) {
  startMatrixGenerator(3, 9, 1, { LATCH, NOT }, "ungeneratable.dot", false,
                       nullptr, false);
}

TEST(MatrixGeneratorTest, Ungeneratable2) {
  startMatrixGenerator(0, 0, 1, { AND, NOT }, "ungeneratable2.dot", false,
                       nullptr, false);
}

TEST(MatrixGeneratorTest, ExtraOuts) {
  startMatrixGenerator(2, 4, 7, { AND, NOT }, "extra_outs.dot", true, nullptr,
                       false);
}

TEST(MatrixGeneratorTest, IrrelevantOps) {
  startMatrixGenerator(1, 2, 1, { NOT, DFF, LATCH }, "irrelevant_ops.dot",
                       false, nullptr, false);
}

TEST(MatrixGeneratorTest, BottomLayerDrain) {
  startMatrixGenerator(1, 4, 2, { NOT, DFF, LATCH }, "bottom_layer_drain.dot",
                       false, nullptr, false);
}

TEST(MatrixGeneratorTest, NoOuts) {
  startMatrixGenerator(1, 2, 0, { AND, NOT }, "no_outs.dot", false, nullptr,
                       false);
}

TEST(MatrixGeneratorTest, AnyNInHandle) {
  startMatrixGenerator(1, 2, 1, { AND, DFF }, "any_n_in_handle.dot", true,
                       nullptr, false);
}

TEST(MatrixGeneratorTest, LinkAllCells) {
  startMatrixGenerator(2, 2, 1, { LATCH, NOT }, "link_all_cells.dot", true,
                       nullptr, false);
}

TEST(MatrixGeneratorTest, MaxOuts) {
  startMatrixGenerator(2, 4, 6, { AND, NOT }, "max_outs.dot", true, nullptr,
                       false);
}

TEST(MatrixGeneratorTest, MAJCells) {
  startMatrixGenerator(30, 1, 1, { MAJ, NOT }, "maj_cells.dot", true, nullptr,
                       false);
}

TEST(MatrixGeneratorTest, LATCHCells) {
  startMatrixGenerator(13, 27, 1, { LATCH, NOT }, "latch_cells.dot", true,
                       nullptr, false);
}

TEST(MatrixGeneratorTest, Less4OpCells) {
  startMatrixGenerator(50, 5, 5, { NOT, AND, OR, XOR, NAND,
                                   NOR, XNOR, MAJ, DFF, LATCH },
                       "less4_op_cells.dot", true, nullptr, false);
}

TEST(MatrixGeneratorTest, SeedUse) {
  startMatrixGenerator(5, 9, 3, { NOT, DFF, LATCH }, "seed_use.dot", true,
                       nullptr, false, 123431);
}

TEST(MatrixGeneratorTest, DFFrsTest) {
  startMatrixGenerator(1, 5, 2, { NOT, DFF, DFFrs }, "dffrs_test.dot", true,
                       nullptr, false);
}

TEST(MatrixGeneratorTest, DFFrsUngeneratable) {
  startMatrixGenerator(1, 4, 2, { NOT, DFF, DFFrs }, "dffrs_ungeneratable.dot",
                       false, nullptr, false);
}

TEST(MatrixGeneratorTest, DFFrsUngeneratable2) {
  startMatrixGenerator(1, 6, 2, { NOT, DFF, DFFrs }, "dffrs_ungeneratable2.dot",
                       false, nullptr, false);
}

TEST(MatrixGeneratorTest, DFFTest) {
  startMatrixGenerator(1, 3, 1, { DFF }, "dff_test.dot", true, nullptr, false);
}

TEST(MatrixGeneratorTest, CustomCell) {
  startMatrixGenerator(1, 2, 1, { createNetCell() }, "custom_cell.dot",
                       true, nullptr, false);
}

TEST(MatrixGeneratorTest, FaninLimit1_5) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 0, 5 };
  startMatrixGenerator(10, 5, 1, { AND, DFFrs }, "fanin_limit_1_5.dot", true,
                       &faninLim, false);
}

TEST(MatrixGeneratorTest, FaninLimit1_2) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 1, 2 };
  startMatrixGenerator(10, 5, 1, { AND }, "fanin_limit_1_2.dot", true,
                       &faninLim, false);
}

TEST(MatrixGeneratorTest, FaninLimit3_5) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 3, 5 };
  startMatrixGenerator(13, 5, 1, { AND, DFFrs }, "fanin_limit_3_5.dot", true,
                       &faninLim, false);
}

TEST(MatrixGeneratorTest, InvalidBasisException) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 3, 5 };
  try {
    startMatrixGenerator(13, 5, 1, { AND, DFFrs, NET },
                         "invalid_basis_exception.dot", true, &faninLim, false);
  } catch (std::invalid_argument &e) {
    EXPECT_STREQ("Generator's base has invalid cell types.", e.what());
    return;
  }
  FAIL();
}

TEST(MatrixGeneratorTest, NetCell) {
  startMatrixGenerator(13, 27, 1, { LATCH, NOT }, "net_cell.dot", true,
                       nullptr, true, 100u);
}

TEST(MatrixGeneratorTest, LimitedNetCell) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 1, 26 };
  startMatrixGenerator(13, 27, 1, { LATCH, NOT }, "limited_net_cell.dot", true,
                       &faninLim, true, 100u);
}

TEST(MatrixGeneratorTest, ManyCells) {
  startMatrixGenerator(10000, 1, 1, { NOT, DFF, LATCH }, "many_cells.dot", true,
                       nullptr, false);
}

// Layer generator tests.

TEST(LayerGeneratorTest, 3Layers) {
  std::vector<int> layerNCells{ 3, 1, 2 };
  startLayerGenerator(9, 2, { NOT, DFF, LATCH }, layerNCells, "3layers.dot",
                      true, nullptr, false);
}

TEST(LayerGeneratorTest, BottomLayerDrain) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(4, 2, { NOT, LATCH }, layerNCells,
                      "bottom_layer_drain.dot", true, nullptr, false);
}

TEST(LayerGeneratorTest, 3BottomLayerDrains) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(6, 4, { NOT, LATCH, DFF }, layerNCells,
                      "3bottom_layer_drains.dot", true, nullptr, false);
}

TEST(LayerGeneratorTest, Ungeneratable) {
  std::vector<int> layerNCells{ 3 };
  startLayerGenerator(9, 1, { AND, NOT }, layerNCells, "ungeneratable.dot",
                      false, nullptr, false);
}

TEST(LayerGeneratorTest, Ungeneratable2) {
  std::vector<int> layerNCells{};
  startLayerGenerator(0, 1, { NOT, LATCH }, layerNCells, "ungeneratable2.dot",
                      false, nullptr, false);
}

TEST(LayerGeneratorTest, ExtraOuts) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(2, 4, { AND }, layerNCells, "extra_outs.dot", true,
                      nullptr, false);
}

TEST(LayerGeneratorTest, IrrelevantOps) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(2, 1, { NOT, DFF, LATCH }, layerNCells,
                      "irrelevant_ops.dot", false, nullptr, false);
}

TEST(LayerGeneratorTest, NoOuts) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(3, 0, { NOT, DFF, LATCH }, layerNCells, "no_outs.dot",
                      false, nullptr, false);
}

TEST(LayerGeneratorTest, MinCells) {
  std::vector<int> layerNCells;
  startLayerGenerator(1, 1, { NOT, DFF, LATCH }, layerNCells, "min_cells.dot",
                      true, nullptr, false);
}

TEST(LayerGeneratorTest, OnlyNot) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(1, 1, { NOT, DFF, LATCH }, layerNCells, "only_not.dot",
                      true, nullptr, false);
}

TEST(LayerGeneratorTest, ANDOp) {
  std::vector<int> layerNCells{ 1, 3, 2, 1 };
  startLayerGenerator(2, 3, { NOT, DFF, AND }, layerNCells, "and_op.dot", true,
                      nullptr, false);
}

TEST(LayerGeneratorTest, SeedUse) {
  std::vector<int> layerNCells(30, 1);
  startLayerGenerator(3, 3, { NOT, DFF, LATCH, AND }, layerNCells,
                      "seed_use.dot", true, nullptr, false, 12314321);
}

TEST(LayerGeneratorTest, Less4OpCells) {
  std::srand(0u);
  std::vector<int> layerNCells(10);
  int nCells = 6;
  for (int i = 0; i < 10; ++i) {
    if (!i) {
      layerNCells[i] = std::rand() % 9 + 2;
      continue;
    }
    int lowerBound = layerNCells[i - 1] / 3 + (layerNCells[i - 1] % 3 ? 1 : 0);
    layerNCells[i] = std::rand() % (6 - (lowerBound - 1)) + lowerBound;
    nCells += layerNCells[i];
  }
  int nOut = std::rand() % (nCells - (layerNCells[9] - 1)) + layerNCells[9];
  startLayerGenerator(6, nOut, { NOT, AND, OR, XOR, NAND,
                                 NOR, XNOR, MAJ, DFF, LATCH }, layerNCells,
                      "less4_op_cells.dot", true, nullptr, false);
}

TEST(LayerGeneratorTest, AnyNInHandle) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(2, 1, { AND, DFF }, layerNCells, "any_n_in_handle.dot",
                      true, nullptr, false);
}

TEST(LayerGeneratorTest, DFFrsTest) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(5, 2, { NOT, DFF, DFFrs }, layerNCells, "dffrs_test.dot",
                      true, nullptr, false);
}

TEST(LayerGeneratorTest, DFFrsUngeneratable) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(4, 1, { NOT, DFF, DFFrs }, layerNCells,
                      "dffrs_ungeneratable.dot", false, nullptr, false);
}

TEST(LayerGeneratorTest, DFFrsUngeneratable2) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(6, 1, { NOT, DFF, DFFrs }, layerNCells,
                      "dffrs_ungeneratable2.dot", false, nullptr, false);
}

TEST(LayerGeneratorTest, DFFTest) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(3, 1, { DFF }, layerNCells, "dff_test.dot", true,
                      nullptr, false);
}

TEST(LayerGeneratorTest, CustomCell) {
  std::vector<int> layerNCells{ 1 };
  startLayerGenerator(2, 1, { createNetCell() }, layerNCells, "custom_cell.dot",
                      true, nullptr, false);
}

TEST(LayerGeneratorTest, FaninLimit1_2) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 1, 2 };
  std::vector<int> layerNCells{ 3, 4, 3 };
  startLayerGenerator(5, 3, { AND }, layerNCells, "fanin_limit_1_2.dot", true,
                      &faninLim, false);
}

TEST(LayerGeneratorTest, FaninLimit3_5) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 3, 5 };
  std::vector<int> layerNCells{ 3, 4, 3, 2, 1 };
  startLayerGenerator(5, 1, { AND, DFFrs }, layerNCells, "fanin_limit_3_5.dot",
                      true, &faninLim, false);
}

TEST(LayerGeneratorTest, FaninLimit1_5) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 0, 5 };
  std::vector<int> layerNCells{ 3, 4, 3, 2, 1 };
  startLayerGenerator(5, 1, { AND, DFFrs }, layerNCells, "fanin_limit_1_5.dot",
                      true, &faninLim, false);
}

TEST(LayerGeneratorTest, FaninLimitException) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 6, 10 };
  std::vector<int> layerNCells{ 3, 4, 3, 2, 1 };
  try {
    startLayerGenerator(5, 1, { AND, DFFrs }, layerNCells,
                        "fanin_lim_exception.dot", true, &faninLim, false);
  } catch (std::invalid_argument &e) {
    EXPECT_STREQ("Generator basis has irrelevant operations.",
                 e.what());
    return;
  }
  FAIL();
}

TEST(LayerGeneratorTest, FaninLimitException2) {
  auto faninLim = std::pair<uint16_t, uint16_t>{ 10, 6 };
  std::vector<int> layerNCells{ 3, 4, 3, 2, 1 };
  try {
    startLayerGenerator(5, 1, { AND, DFFrs }, layerNCells,
                        "fanin_lim_exception2.dot", true, &faninLim, false);
  } catch (std::invalid_argument &e) {
    EXPECT_STREQ("Fanin lower bound is greater than fanin upper bound.",
                 e.what());
    return;
  }
  FAIL();
}

TEST(LayerGeneratorTest, InvalidBasisException) {
  std::vector<int> layerNCells{ 3, 4, 3, 2, 1 };
  try {
    startLayerGenerator(5, 1, { AND, DFFrs, IN }, layerNCells,
                        "invalid_basis_exception.dot", true, nullptr, false);
  } catch (std::exception &e) {
    EXPECT_STREQ("Generator's base has invalid cell types.",
                 e.what());
    return;
  }
  FAIL();
}

TEST(LayerGeneratorTest, NetCell) {
  std::vector<int> layerNCells{ 1, 3, 2, 1 };
  startLayerGenerator(2, 3, { NOT, DFF, AND }, layerNCells, "net_cell.dot",
                      true, nullptr, true, 100u);
}

TEST(LayerGeneratorTest, LimitedNetCell) {
  std::vector<int> layerNCells{ 2, 3, 2, 1 };
  auto faninLim = std::pair<uint16_t, uint16_t>{ 1, 9 };
  startLayerGenerator(10, 3, { NOT, DFF, AND }, layerNCells,
                      "limited_net_cell.dot", true, &faninLim, true, 100u);
}

TEST(LayerGeneratorTest, ManyCells) {
  std::vector<int> layerNCells(100000, 1);
  startLayerGenerator(1, 1, { NOT, DFF, LATCH }, layerNCells, "many_cells.dot",
                      true, nullptr, false);
}

// General tests

TEST(GeneratorsTest, GeneratorName) {
  const auto netBase = { NOT, DFF, LATCH };
  std::vector<int> layerNCells(100000, 1);

  auto layerGenerator = LayerGenerator(1, 1, netBase, layerNCells);
  auto matrixGenerator = MatrixGenerator(1, 1, 1, netBase);

  std::vector<Generator *> generators{ &layerGenerator, &matrixGenerator };

  for (std::size_t i = 0; i < generators.size(); ++i) {
    Generator *generator = generators[i];
    const std::string &curGeneratorName = generator->getName();
    if (!i) {
      EXPECT_STREQ("LayerGenerator", curGeneratorName.c_str());
    } else {
      EXPECT_STREQ("MatrixGenerator", curGeneratorName.c_str());
    }
  }
}

} // namespace eda::gate::model
