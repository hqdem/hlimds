//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library.h"
#include "gate/library/readcells_srcfile_parser.h"
#include "gate/library/library_factory.h"
#include "gate/library/library_index.h"
#include "util/env.h"

#include "gate/techmapper/utils/read_sdc.h"

#include "gtest/gtest.h"

#include <vector>

using path = std::filesystem::path;

const path home = eda::env::getHomePath();
const path techLibPath = home /
                     "test/data/gate/techmapper";

namespace eda::gate::library {

//Example1: index element is a pointer
using SelectedTypeIsPtr = const StandardCell*;
using SelectedVectorOfP = SCLibraryIndex<SelectedTypeIsPtr>::SelectionType;

SelectedVectorOfP selectAllCellsAsPtrs(const SCLibrary& library) {
  SelectedVectorOfP storage;
  for (auto& cell: library.getCombCells()) {
    storage.push_back(&cell);
  }
  return storage;
}
struct SelectAllCellsFunctor {
  const SCLibrary& library;

  SelectedVectorOfP operator()() {
    return selectAllCellsAsPtrs(library);
  }
};

//Example2: index element is a shallow copy
using SelectedTypeIsVal = StandardCell;
using SelectedVectorV = SCLibraryIndex<SelectedTypeIsVal>::SelectionType;

SelectedVectorV selectAllCellsAsShallowCopyVal(const SCLibrary& library,
                                              int arg) {
  (void)arg; //Example of passing arguments
  SelectedVectorV storage;
  for (auto& cell: library.getCombCells()) {
    storage.push_back(cell);
  }
  return storage;
}

using SelectedTypeIsTemplPtr = const LutTemplate*;
using SelectedVectorOfTemplP = SCLibraryIndex<SelectedTypeIsTemplPtr>::SelectionType;

SelectedVectorOfTemplP selectAllTemplsAsPtrs(const SCLibrary& library) {
  SelectedVectorOfTemplP storage;
  for (auto& templ: library.getTemplates()) {
    storage.push_back(&templ);
  }
  return storage;
}

bool checkLibParser2(std::string libertyPath) {
  ReadCellsParser parser(libertyPath);
  SCLibrary library = SCLibraryFactory::newLibrary(parser);
  std::cout << "Loaded Liberty: " << libertyPath << std::endl;

  #ifdef UTOPIA_DEBUG
  for(const auto& cell : library.getCells()) {
    std::cout << gate::model::CellType::get(cell.cellTypeID).getName()
              << std::endl;
  }
  #endif // UTOPIA_DEBUG

  SCLibraryIndex<SelectedTypeIsPtr> index1 (SelectAllCellsFunctor{library});
  SCLibraryIndex<SelectedTypeIsPtr> index2 (
    [&]{return selectAllCellsAsPtrs(library);});
  SCLibraryIndex<SelectedTypeIsVal> index3 (
    [&]{return selectAllCellsAsShallowCopyVal(library, 0);});
  SCLibraryIndex<SelectedTypeIsTemplPtr> index4 (
    [&]{return selectAllTemplsAsPtrs(library);});
  for(auto cell : index1) {
    std::cout << gate::model::CellType::get(cell->cellTypeID).getName()
              << std::endl;
  }
  for(auto &templ : index4) {
    std::cout << "Template:\n";
    for (auto &var: templ->variables) {
      std::cout << "Variable ID: " << std::setw(2)
                << static_cast<int>(var.nameID) << " | Values: ";
      for (auto &val : var.values) {
        std::cout << val << " ";
      }
      std::cout <<"\n";
    }
    std::cout <<"\n";
  }
  return true;
}

const std::tuple<float, float, float> checkSDCParser(std::string sdcPath) {
  const auto constraints = techmapper::parseSDCFile(sdcPath);
#ifdef UTOPIA_DEBUG
  std::cout << "Max delay: " << std::get<0>(constraints) << std::endl;
  std::cout << "Max area: " << std::get<1>(constraints) << std::endl;
  std::cout << "Max power: " << std::get<2>(constraints) << std::endl;
#endif // UTOPIA_DEBUG
  return constraints;
}

TEST(ReadLibertyTest2, sky130_fd_sc_hd__ff_n40C_1v95) {
  checkLibParser2(techLibPath / "sky130_fd_sc_hd__ff_n40C_1v95.lib");
}

TEST(ReadLibertyTest2, sky130_fd_sc_hd__ff_100C_1v65) {
  checkLibParser2(techLibPath / "sky130_fd_sc_hd__ff_100C_1v65.lib");
}

TEST(ReadSDCTest, test_100) {
  const auto &constraints = checkSDCParser(techLibPath / "test.sdc");

  ASSERT_EQ(std::get<0>(constraints), 100);
  ASSERT_EQ(std::get<1>(constraints), 100);
  ASSERT_EQ(std::get<2>(constraints), 100);
}

} // namespace eda::gate::techmapper
