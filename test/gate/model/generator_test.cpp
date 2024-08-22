//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#define FANOUTNOTCHECK

#include "gate/model/generator/matrix_generator.h"
#include "gate/model/generator/layer_generator.h"
#include "gate/model/printer/net_printer.h"
#include "test_util.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

namespace eda::gate::model {

const std::string testOutPath = "data/gate/optimizer/generator/";
const std::string matrixGenSubfolder = "matrix/";
const std::string layerGenSubfolder = "layer/";

bool netValid(const Net &net,
              const std::size_t nCells,
              const std::size_t nIn,
              const std::size_t nOut,
              const bool generatable,
              const std::size_t nestingDepth);

bool checkCellsValid(const List<CellID> &cells,
                     const std::size_t nCells,
                     const std::size_t nIn,
                     const std::size_t nOut,
                     const bool generatable,
                     const std::size_t nestingDepth) {

  for (const auto &cellId : cells) {
    const Cell &cell = Cell::get(cellId);
    const CellType &cellType = cell.getType();
    const uint16_t cellFanin = cell.getFanin();
#ifndef FANOUTNOTCHECK
    const uint16_t cellFanout = cell.getFanout();
    const CellSymbol &cellSymbol = cell.getType().getSymbol();
#endif
    if ((!cellType.isInNumFixed() && cellFanin < 2) &&
        (cellType.getInNum() != cellFanin)) {
      return false;
    }
#ifndef FANOUTNOTCHECK
    if ((cellSymbol == OUT && cellFanout > 0) ||
        (cellSymbol != OUT && cellFanout == 0)) {
      return false;
    }
#endif
    if (!cellType.isNet()) {
      continue;
    }
    const auto &cellNet = cellType.getNet();
    if (!netValid(cellNet, nCells, nIn, nOut, generatable,
                  nestingDepth - 1)) {
      return false;
    }
  }
  return true;
}

bool netValid(const Net &net,
              const std::size_t nCells,
              const std::size_t nIn,
              const std::size_t nOut,
              const bool generatable,
              const std::size_t nestingDepth = 1) {

  const uint16_t netInN = net.getInNum(), netOutN = net.getOutNum();
  const uint32_t netCombN = net.getCombNum();
  if (!(netInN == nIn && netOutN == nOut &&
        (netCombN + net.getFlipNum()) +
        net.getSoftNum() + net.getHardNum() == nCells) ||
      (!nestingDepth && (net.getHardNum() || net.getSoftNum()))) {

    return false;
  }

  return checkCellsValid(net.getCombCells(), nCells, nIn, nOut, generatable,
                         nestingDepth) &&
         checkCellsValid(net.getFlipFlops(), nCells, nIn, nOut, generatable,
                         nestingDepth) &&
         checkCellsValid(net.getOutputs(), nCells, nIn, nOut, generatable,
                         nestingDepth) &&
         checkCellsValid(net.getInputs(), nCells, nIn, nOut, generatable,
                         nestingDepth) &&
         checkCellsValid(net.getHardBlocks(), nCells, nIn, nOut, generatable,
                         nestingDepth) &&
         checkCellsValid(net.getSoftBlocks(), nCells, nIn, nOut, generatable,
                         nestingDepth);
}

bool netValid(const NetID netID,
              const std::size_t nCells,
              const std::size_t nIn,
              const std::size_t nOut,
              const bool generatable,
              const std::size_t nestingDepth = 1) {

  if (netID == OBJ_NULL_ID) {
    return !generatable;
  }
  if (!generatable) {
    return false;
  }
  const auto &net = Net::get(netID);
  return netValid(net, nCells, nIn, nOut, generatable, nestingDepth);
}

std::size_t getNCells(const std::vector<std::size_t> &layerNCells) {
  std::size_t nCells = 0;
  for (std::size_t i = 0; i < layerNCells.size(); ++i) {
    nCells += layerNCells[i];
  }
  return nCells;
}

std::size_t getNCells(const NetID netID) {
  if (netID == OBJ_NULL_ID) {
    return 0;
  }
  const auto &net = Net::get(netID);
  std::size_t nCells = net.getCombNum() + net.getFlipNum() + net.getHardNum() +
                       net.getSoftNum();
  return nCells;
}

void printGeneratedNet(const NetID netID,
                       const std::string &subFolder,
                       const std::string &fileName) {

  if (netID == OBJ_NULL_ID) {
    return;
  }
  const auto &net = Net::get(netID);
  std::ofstream out;
  std::filesystem::path filePath = createOutDir(testOutPath + subFolder);
  out.open(filePath.c_str() + fileName);
  model::print(out, model::DOT, net);
  out.close();
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

  return makeCellType(UNDEF, "net", netBuilder.make(), OBJ_NULL_ID,
                      CellProperties(0, 1, 1, 0, 0, 0, 0, 0, 0), 2, 1);
}

// Matrix generator tests.

TEST(MatrixGeneratorTest, MinCells) {
  MatrixGenerator generator(0, 1, 1, { AND, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 0, 1, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "min_cells.dot");
}

TEST(MatrixGeneratorTest, OnlyNot) {
  MatrixGenerator generator(1, 1, 1, { NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 1, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "only_not.dot");
}

TEST(MatrixGeneratorTest, SeveralIn) {
  MatrixGenerator generator(40, 30, 1, { AND, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 40, 30, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "several_in.dot");
}

TEST(MatrixGeneratorTest, SeveralOut) {
  MatrixGenerator generator(40, 1, 30, { AND, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 40, 1, 30, true));
  printGeneratedNet(netID, matrixGenSubfolder, "several_out.dot");
}

TEST(MatrixGeneratorTest, SeveralInOut) {
  MatrixGenerator generator(40, 10, 50, { AND, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 40, 10, 50, true));
  printGeneratedNet(netID, matrixGenSubfolder, "several_in_out.dot");
}

TEST(MatrixGeneratorTest, Ungeneratable) {
  MatrixGenerator generator(3, 9, 1, { DLATCH, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 3, 9, 1, false));
  printGeneratedNet(netID, matrixGenSubfolder, "ungeneratable.dot");
}

TEST(MatrixGeneratorTest, Ungeneratable2) {
  MatrixGenerator generator(0, 0, 1, { AND, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 0, 0, 1, false));
  printGeneratedNet(netID, matrixGenSubfolder, "ungeneratable2.dot");
}

TEST(MatrixGeneratorTest, ExtraOuts) {
  MatrixGenerator generator(2, 4, 7, { AND, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 2, 4, 7, true));
  printGeneratedNet(netID, matrixGenSubfolder, "extra_outs.dot");
}

TEST(MatrixGeneratorTest, IrrelevantOps) {
  MatrixGenerator generator(1, 3, 1, { NOT, DFF, DLATCH });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 3, 1, false));
  printGeneratedNet(netID, matrixGenSubfolder, "irrelevant_ops.dot");
}

TEST(MatrixGeneratorTest, BottomLayerDrain) {
  MatrixGenerator generator(1, 4, 2, { NOT, DFF, DLATCH });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 4, 2, false));
  printGeneratedNet(netID, matrixGenSubfolder, "bottom_layer_drain.dot");
}

TEST(MatrixGeneratorTest, NoOuts) {
  MatrixGenerator generator(1, 2, 0, { NOT, AND });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 2, 0, false));
  printGeneratedNet(netID, matrixGenSubfolder, "no_outs.dot");
}

TEST(MatrixGeneratorTest, AnyNInHandle) {
  MatrixGenerator generator(1, 2, 1, { AND, DFF });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 2, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "any_n_in_handle.dot");
}

TEST(MatrixGeneratorTest, LinkAllCells) {
  MatrixGenerator generator(2, 2, 1, { DLATCH, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 2, 2, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "link_all_cells.dot");
}

TEST(MatrixGeneratorTest, TwoOutsForCell) {
  MatrixGenerator generator(2, 4, 7, { AND, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 2, 4, 7, true));
  printGeneratedNet(netID, matrixGenSubfolder, "two_outs_for_cell.dot");
}

TEST(MatrixGeneratorTest, MAJCells) {
  MatrixGenerator generator(30, 1, 1, { MAJ, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 30, 1, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "maj_cells.dot");
}

TEST(MatrixGeneratorTest, LATCHCells) {
  MatrixGenerator generator(27, 28, 1, { DLATCH, NOT });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 27, 28, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "latch_cells.dot");
}

TEST(MatrixGeneratorTest, Less4OpCells) {
  MatrixGenerator generator(50, 5, 5,
      { NOT, AND, OR, XOR, NAND, NOR, XNOR, MAJ, DFF, DLATCH });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 50, 5, 5, true));
  printGeneratedNet(netID, matrixGenSubfolder, "less4_op_cells.dot");
}

TEST(MatrixGeneratorTest, SeedUse) {
  MatrixGenerator generator(10, 9, 3, { NOT, DFF, DLATCH }, 123431);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 10, 9, 3, true));
  printGeneratedNet(netID, matrixGenSubfolder, "seed_use.dot");
}

TEST(MatrixGeneratorTest, DFFrsTest) {
  MatrixGenerator generator(1, 4, 2, { NOT, DFF, DFFrs });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 4, 2, true));
  printGeneratedNet(netID, matrixGenSubfolder, "dffrs_test.dot");
}

TEST(MatrixGeneratorTest, DFFrsUngeneratable) {
  MatrixGenerator generator(1, 3, 2, { NOT, DFF, DFFrs });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 3, 2, false));
  printGeneratedNet(netID, matrixGenSubfolder, "dffrs_ungeneratable.dot");
}

TEST(MatrixGeneratorTest, DFFrsUngeneratable2) {
  MatrixGenerator generator(1, 6, 2, { NOT, DFF, DFFrs });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 6, 2, false));
  printGeneratedNet(netID, matrixGenSubfolder, "dffrs_ungeneratable2.dot");
}

TEST(MatrixGeneratorTest, CustomCell) {
  MatrixGenerator generator(1, 2, 1, { createNetCell() });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 2, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "custom_cell.dot");
}

TEST(MatrixGeneratorTest, FaninLimit1_5) {
  MatrixGenerator generator(10, 5, 1, { AND, DFFrs });
  generator.setFaninHigh(5);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 10, 5, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "fanin_limit_1_5.dot");
}

TEST(MatrixGeneratorTest, FaninLimit1_2) {
  MatrixGenerator generator(10, 5, 1, { AND });
  generator.setFaninLim(1, 2);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 10, 5, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "fanin_limit_1_2.dot");
}

TEST(MatrixGeneratorTest, FaninLimit3_5) {
  MatrixGenerator generator(13, 5, 1, { AND, DFFrs });
  generator.setFaninLim(3, 5);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 13, 5, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "fanin_limit_3_5.dot");
}

TEST(MatrixGeneratorTest, InvalidBasisException) {
  try {
    MatrixGenerator generator(13, 5, 1, { AND, DFFrs, UNDEF });
    generator.setFaninLim(3, 5);
  } catch (std::invalid_argument &e) {
    EXPECT_STREQ("Generator's base has invalid cell types.", e.what());
    return;
  }
  FAIL();
}

TEST(MatrixGeneratorTest, NetCell) {
  MatrixGenerator generator(27, 28, 1, { DLATCH, NOT }, 100u);
  generator.setHierarchical(true);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 27, 28, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "net_cell.dot");
}

TEST(MatrixGeneratorTest, NestingDepthLim) {
  MatrixGenerator generator(1, 1, 1, { NOT }, 100u);
  generator.setHierarchical(true);
  generator.setNestingMax(10);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 1, 1, 1, true, 10));
  printGeneratedNet(netID, matrixGenSubfolder, "nesting_depth_lim.dot");
}

TEST(MatrixGeneratorTest, LimitedNetCell) {
  MatrixGenerator generator(27, 28, 1, { DLATCH, NOT }, 100u);
  generator.setHierarchical(true);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 27, 28, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "limited_net_cell.dot");
}

TEST(MatrixGeneratorTest, ManyCells) {
  MatrixGenerator generator(10000, 1, 1, { DLATCH, NOT, DFF });
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, 10000, 1, 1, true));
  printGeneratedNet(netID, matrixGenSubfolder, "many_cells.dot");
}

// Layered generator with layerNcells constructor tests.

TEST(LayerGeneratorTest, 3Layers) {
  std::vector<std::size_t> layerNCells{ 5, 3, 2 };
  LayerGenerator generator(9, 2, { NOT, DFF, DLATCH }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 9, 2, true));
  printGeneratedNet(netID, layerGenSubfolder, "3layers.dot");
}

TEST(LayerGeneratorTest, BottomLayerDrain) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(3, 2, { NOT, DLATCH }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 3, 2, true));
  printGeneratedNet(netID, layerGenSubfolder, "bottom_layer_drain.dot");
}

TEST(LayerGeneratorTest, 3BottomLayerDrains) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(5, 4, { NOT, DLATCH, DFF }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 5, 4, true));
  printGeneratedNet(netID, layerGenSubfolder, "3bottom_layer_drains.dot");
}

TEST(LayerGeneratorTest, Ungeneratable) {
  std::vector<std::size_t> layerNCells{ 3 };
  LayerGenerator generator(9, 1, { NOT, AND }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 9, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "ungeneratable.dot");
}

TEST(LayerGeneratorTest, Ungeneratable2) {
  std::vector<std::size_t> layerNCells{};
  LayerGenerator generator(0, 1, { NOT, DLATCH }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 0, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "ungeneratable2.dot");
}

TEST(LayerGeneratorTest, ExtraOuts) {
  std::vector<std::size_t> layerNCells{ 3 };
  LayerGenerator generator(2, 4, { AND }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 2, 4, true));
  printGeneratedNet(netID, layerGenSubfolder, "extra_outs.dot");
}

TEST(LayerGeneratorTest, IrrelevantOps) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(1, 1, { DFF, DLATCH }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 1, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "irrelevant_ops.dot");
}

TEST(LayerGeneratorTest, NoOuts) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(3, 0, { NOT, DFF, DLATCH }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 3, 0, false));
  printGeneratedNet(netID, layerGenSubfolder, "no_outs.dot");
}

TEST(LayerGeneratorTest, MinCells) {
  std::vector<std::size_t> layerNCells;
  LayerGenerator generator(1, 1, { NOT, DFF, DLATCH }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 1, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "min_cells.dot");
}

TEST(LayerGeneratorTest, OnlyNot) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(1, 1, { NOT, DFF, DLATCH }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 1, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "only_not.dot");
}

TEST(LayerGeneratorTest, ANDOp) {
  std::vector<std::size_t> layerNCells{ 1, 3, 2, 1 };
  LayerGenerator generator(1, 1, { NOT, DFF, AND }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 1, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "and_op.dot");
}

TEST(LayerGeneratorTest, SeedUse) {
  std::vector<std::size_t> layerNCells(30, 1);
  LayerGenerator generator(3, 3, { NOT, DFF, DLATCH, AND }, layerNCells,
                           12314321);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 3, 3, true));
  printGeneratedNet(netID, layerGenSubfolder, "seed_use.dot");
}

TEST(LayerGeneratorTest, Less4OpCells) {
  std::srand(0u);
  std::vector<std::size_t> layerNCells(10);
  std::size_t nCells = 6;
  for (std::size_t i = 0; i < 10; ++i) {
    if (!i) {
      layerNCells[i] = std::rand() % 9 + 2;
      continue;
    }
    uint16_t lowerBound = layerNCells[i - 1] / 3 +
                          (layerNCells[i - 1] % 3 ? 1 : 0);
    layerNCells[i] = std::rand() % (6 - (lowerBound - 1)) + lowerBound;
    nCells += layerNCells[i];
  }
  std::size_t nOut = std::rand() % (nCells - (layerNCells[9] - 1)) +
                     layerNCells[9];

  LayerGenerator generator(6, nOut,
      { NOT, AND, OR, XOR, NAND, NOR, XNOR, MAJ, DFF, DLATCH }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 6, nOut, true));
  printGeneratedNet(netID, layerGenSubfolder, "less4_op_cells.dot");
}

TEST(LayerGeneratorTest, AnyNInHandle) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(2, 1, { DFF, AND }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 2, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "any_n_in_handle.dot");
}

TEST(LayerGeneratorTest, DFFrsTest) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(5, 2, { DFF, AND, DFFrs }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 5, 2, true));
  printGeneratedNet(netID, layerGenSubfolder, "dffrs_test.dot");
}

TEST(LayerGeneratorTest, DFFrsUngeneratable) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(3, 1, { DFF, NOT, DFFrs }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 3, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "dffrs_ungeneratable.dot");
}

TEST(LayerGeneratorTest, DFFrsUngeneratable2) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(6, 1, { DFF, NOT, DFFrs }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 6, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "dffrs_ungeneratable2.dot");
}

TEST(LayerGeneratorTest, DFFTest) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(2, 1, { DFF }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 2, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "dff_test.dot");
}

TEST(LayerGeneratorTest, CustomCell) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(2, 1, { createNetCell() }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 2, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "custom_cell.dot");
}

TEST(LayerGeneratorTest, FaninLimit1_2) {
  std::vector<std::size_t> layerNCells{ 3, 4, 3 };
  LayerGenerator generator(5, 3, { AND }, layerNCells);
  generator.setFaninLim(1, 2);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 5, 3, true));
  printGeneratedNet(netID, layerGenSubfolder, "fanin_limit_1_2.dot");
}

TEST(LayerGeneratorTest, FaninLimit3_5) {
  std::vector<std::size_t> layerNCells{ 3, 4, 3, 2, 1 };
  LayerGenerator generator(5, 1, { AND, DFFrs }, layerNCells);
  generator.setFaninLim(3, 5);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 5, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "fanin_limit_3_5.dot");
}

TEST(LayerGeneratorTest, FaninLimit1_5) {
  std::vector<std::size_t> layerNCells{ 3, 4, 3, 2, 1 };
  LayerGenerator generator(5, 1, { AND, DFFrs }, layerNCells);
  generator.setFaninHigh(5);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 5, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "fanin_limit_1_5.dot");
}

TEST(LayerGeneratorTest, FaninLimitException) {
  std::vector<std::size_t> layerNCells{ 3, 4, 3, 2, 1 };
  LayerGenerator generator(5, 1, { AND, DFFrs }, layerNCells);
  try {
    generator.setFaninLim(6, 10);
  } catch (std::invalid_argument &e) {
    EXPECT_STREQ("Generator basis has irrelevant operations.",
                 e.what());
    return;
  }
  FAIL();
}

TEST(LayerGeneratorTest, FaninLimitException2) {
  std::vector<std::size_t> layerNCells{ 3, 4, 3, 2, 1 };
  LayerGenerator generator(5, 1, { AND, DFFrs }, layerNCells);
  try {
    generator.setFaninLim(10, 6);
  } catch (std::invalid_argument &e) {
    EXPECT_STREQ("Fanin lower bound is greater than fanin upper bound.",
                 e.what());
    return;
  }
  FAIL();
}

TEST(LayerGeneratorTest, InvalidBasisException) {
  std::vector<std::size_t> layerNCells{ 3, 4, 3, 2, 1 };
  try {
    LayerGenerator generator(5, 1, { AND, DFFrs, IN }, layerNCells);
  } catch (std::invalid_argument &e) {
    EXPECT_STREQ("Generator's base has invalid cell types.",
                 e.what());
    return;
  }
  FAIL();
}

TEST(LayerGeneratorTest, NetCell) {
  std::vector<std::size_t> layerNCells{ 1, 3, 2, 1 };
  LayerGenerator generator(2, 3, { NOT, DFF, AND }, layerNCells);
  generator.setHierarchical(true);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 2, 3, true));
  printGeneratedNet(netID, layerGenSubfolder, "net_cell.dot");
}

TEST(LayerGeneratorTest, NestingDepthLim) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(1, 1, { NOT }, layerNCells, 658u);
  generator.setHierarchical(true);
  generator.setNestingMax(2);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 1, 1, true, 2));
  printGeneratedNet(netID, layerGenSubfolder, "nesting_depth_lim.dot");
}

TEST(LayerGeneratorTest, NestingDepthLim2) {
  std::vector<std::size_t> layerNCells{ 2, 1 };
  LayerGenerator generator(1, 1, { NOT, AND }, layerNCells, 1u);
  generator.setHierarchical(true);
  generator.setNestingMax(0);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 1, 1, true, 1));
  printGeneratedNet(netID, layerGenSubfolder, "nesting_depth_lim2.dot");
}

TEST(LayerGeneratorTest, LimitedNetCell) {
  std::vector<std::size_t> layerNCells{ 2, 3, 2, 1 };
  LayerGenerator generator(10, 3, { NOT, AND, DFF }, layerNCells, 100u);
  generator.setHierarchical(true);
  generator.setFaninHigh(9);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 10, 3, true, 9));
  printGeneratedNet(netID, layerGenSubfolder, "limited_net_cell.dot");
}

TEST(LayerGeneratorTest, UngeneratableNetCell) {
  std::vector<std::size_t> layerNCells{ 1 };
  LayerGenerator generator(4, 1, { DFF }, layerNCells);
  generator.setHierarchical(true);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 4, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "ungeneratable_net_cell.dot");
}

TEST(LayerGeneratorTest, ManyCells) {
  std::vector<std::size_t> layerNCells(10000, 1);
  LayerGenerator generator(1, 1, { NOT, DFF, DLATCH }, layerNCells);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(layerNCells), 1, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "many_cells.dot");
}

// Layered generator with nLayers constructor tests.

TEST(LayerGeneratorTest, FixedNCellsOnLayer) {
  LayerGenerator generator(4, 1, { DLATCH }, 2, 1, 2);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 4, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "fixed_layer_n_cells.dot");
}

TEST(LayerGeneratorTest, RandomNCellsOnLayer) {
  LayerGenerator generator(9, 1, { DLATCH, AND }, 10, 1, 7);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 9, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "random_layer_n_cells.dot");
}

TEST(LayerGeneratorTest, UngeneratableNLayers) {
  LayerGenerator generator(4, 1, { DLATCH }, 1, 1, 1);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 4, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "ungeneratable_n_layers.dot");
}

TEST(LayerGeneratorTest, UngeneratableNLayers2) {
  LayerGenerator generator(4, 1, { DFF }, 1, 1, 2);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 4, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "ungeneratable_n_layers2.dot");
}

TEST(LayerGeneratorTest, UngeneratableNLayers3) {
  LayerGenerator generator(7, 1, { DFF }, 2, 1, 1);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 7, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "ungeneratable_n_layers3.dot");
}

TEST(LayerGeneratorTest, ZeroLayers) {
  LayerGenerator generator(1, 1, { DFF }, 0, 1, 100);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 1, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "zero_layers.dot");
}

TEST(LayerGeneratorTest, LayerNCellsMinTest) {
  LayerGenerator generator(10, 3, { DFF, DLATCH, DFFrs, NOT }, 5, 3, 5);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 10, 3, true));
  printGeneratedNet(netID, layerGenSubfolder, "layer_n_cells_min_test.dot");
}

TEST(LayerGeneratorTest, HierarchicalNLayers) {
  LayerGenerator generator(6, 4, { NOT, DFF }, 2, 1, 4);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 6, 4, true));
  printGeneratedNet(netID, layerGenSubfolder, "hierarchical_n_layers.dot");
}

TEST(LayerGeneratorTest, FaninLimNLayers) {
  LayerGenerator generator(8, 4, { NOT, DFF }, 1, 1, 4);
  generator.setHierarchical(true);
  generator.setNestingMax(2);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 8, 4, true, 2));
  printGeneratedNet(netID, layerGenSubfolder, "fanin_lim_n_layers.dot");
}

TEST(LayerGeneratorTest, UngeneratableNetCell2) {
  LayerGenerator generator(1, 1, { DFF }, 1, 1, 1);
  generator.setHierarchical(true);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 1, 1, false));
  printGeneratedNet(netID, layerGenSubfolder, "ungeneratable_net_cell2.dot");
}

TEST(LayerGeneratorTest, ManyCellsNLayers) {
  LayerGenerator generator(1, 1, { NOT, DFF, DLATCH }, 10000, 1, 1);
  const auto netID = generator.generate();
  EXPECT_TRUE(netValid(netID, getNCells(netID), 1, 1, true));
  printGeneratedNet(netID, layerGenSubfolder, "many_cells_n_layers.dot");
}

// General tests

TEST(GeneratorsTest, GeneratorName) {
  const auto netBase = { NOT, DFF, DLATCH };
  std::vector<std::size_t> layerNCells(100000, 1);

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
