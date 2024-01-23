//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/net.h"
#include "gate/model2/printer/dot.h"
#include "gate/model2/printer/printer.h"
#include "gate/optimizer/optimizer_util.h"

#include "gtest/gtest.h"

#include <fstream>
#include <iostream>
#include <vector>

namespace eda::gate::model {

/**
 *  \brief Generates a sequential circuit that contains a flip-flop.
 *
 *  The circuit consist of AND, OR, NOT, XOR, LATCH cells. The circuit does not
 *  contain cycles.
 *
 */
const Net &genSeqNet() {
  NetBuilder netBuilder;
  CellID tmp;
  CellID cells[100];
  for (size_t i = 0; i < 4; i++) {
    tmp = makeCell(IN);
    cells[i] = tmp;
    netBuilder.addCell(tmp);
  }

  for (size_t i = 0; i < 2; ++i) {

    cells[4 + i * 4] = makeCell(AND, cells[0], cells[2]);
    netBuilder.addCell(cells[4 + i * 4]);

    cells[5 + i * 4] = makeCell(OR, cells[1], cells[3]);
    netBuilder.addCell(cells[5 + i * 4]);

    cells[6 + i * 4] = makeCell(AND, cells[1], cells[2]);
    netBuilder.addCell(cells[6 + i * 4]);

    cells[7 + i * 4] = makeCell(OR, cells[3], cells[0]);
    netBuilder.addCell(cells[7 + i * 4]);

    cells[12 + i * 4] = makeCell(NOT, cells[4 + i * 4]);
    netBuilder.addCell(cells[12 + i * 4]);

    cells[13 + i * 4] = makeCell(AND, cells[12 + i * 4], cells[6 + i * 4]);
    netBuilder.addCell(cells[13 + i * 4]);

    cells[14 + i * 4] = makeCell(OR, cells[4 + i * 4], cells[5 + i * 4]);
    netBuilder.addCell(cells[14 + i * 4]);

    cells[15 + i * 4] = makeCell(OR, cells[4 + i * 4], cells[7 + i * 4]);
    netBuilder.addCell(cells[15 + i * 4]);

    cells[20 + i * 4] = makeCell(AND, cells[13 + i * 4], cells[14 + i * 4]);
    netBuilder.addCell(cells[20 + i * 4]);

    cells[21 + i * 4] = makeCell(OR, cells[15 + i * 4], cells[5 + i * 4]);
    netBuilder.addCell(cells[21 + i * 4]);

    cells[22 + i * 4] = makeCell(AND, cells[15 + i * 4], cells[6 + i * 4]);
    netBuilder.addCell(cells[22 + i * 4]);

    cells[23 + i * 4] = makeCell(AND, cells[7 + i * 4], cells[6 + i * 4]);
    netBuilder.addCell(cells[23 + i * 4]);

    cells[28 + i * 4] = makeCell(AND, cells[13 + i * 4], cells[20 + i * 4]);
    netBuilder.addCell(cells[28 + i * 4]);

    cells[29 + i * 4] = makeCell(OR, cells[21 + i * 4], cells[14 + i * 4]);
    netBuilder.addCell(cells[29 + i * 4]);

    cells[30 + i * 4] = makeCell(OR, cells[21 + i * 4], cells[20 + i * 4]);
    netBuilder.addCell(cells[30 + i * 4]);

    cells[31 + i * 4] = makeCell(OR, cells[22 + i * 4], cells[23 + i * 4]);
    netBuilder.addCell(cells[31 + i * 4]);

    cells[36 + i * 2] = makeCell(OR, cells[28 + i * 4], cells[29 + i * 4]);
    netBuilder.addCell(cells[36 + i * 2]);

    cells[37 + i * 2] = makeCell(OR, cells[30 + i * 4], cells[31 + i * 4]);
    netBuilder.addCell(cells[37 + i * 2]);

    cells[40 + i] = makeCell(OR, cells[36 + i * 2], cells[37 + i * 2]);
    netBuilder.addCell(cells[40 + i]);
  }

  cells[42] = makeCell(XOR, cells[40], cells[41]);
  netBuilder.addCell(cells[42]);

  cells[43] = makeCell(XOR, cells[40], cells[41]);
  netBuilder.addCell(cells[43]);

  cells[44] = makeCell(NOT, cells[42]);
  netBuilder.addCell(cells[44]);

  cells[45] = makeCell(LATCH, cells[43], cells[44]);
  netBuilder.addCell(cells[45]);

  cells[46] = makeCell(OUT, cells[44]);
  netBuilder.addCell(cells[46]);

  cells[47] = makeCell(OUT, cells[45]);
  netBuilder.addCell(cells[47]);

  return Net::get(netBuilder.make());
}

TEST(SeqNet, netWithLatch) {

  const Net &net = genSeqNet();
  ModelPrinter *dotPrinter = &ModelPrinter::getDefaultPrinter();

  if (!getenv("UTOPIA_HOME")) {
    FAIL() << "UTOPIA_HOME is not set.";
  }
  const std::string homePath = getenv("UTOPIA_HOME");
  const std::string outFolder = "/output/test/model2/";
  const std::string fileName = "net_with_latch.dot";
  std::filesystem::path filePath =
      optimizer::createOutPath(homePath + outFolder);

  std::ofstream out(filePath.c_str() + fileName);
  if (out.is_open()) {
    dotPrinter->print(out, net);
    out.close();
    EXPECT_TRUE(std::filesystem::exists(filePath.c_str() + fileName));
    EXPECT_TRUE(std::filesystem::file_size(filePath.c_str() + fileName) > 0);
  }
}

} // namespace eda::gate::model
