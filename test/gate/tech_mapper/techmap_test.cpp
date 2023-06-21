
//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/gate_verilog_parser.h"
#include "gate/printer/dot.h"
#include "gate/optimizer/examples.h"

#include "gate/tech_mapper/parser_lib_test.h"
#include "gate/tech_mapper/tech_mapper.h"
#include "gate/tech_mapper/strategy/simple_techmapper.h"
#include "gate/tech_mapper/strategy/replacement_cut.h"

#include "gate/premapper/mapper/mapper_test.h"
#include "gate/debugger/checker.h"


#include "gtest/gtest.h"

namespace eda::gate::optimizer {
  using lorina::text_diagnostics;
  using lorina::diagnostic_engine;
  using lorina::return_code;

  const std::filesystem::path homePathTechMap = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path libertyDirrectTechMap = homePathTechMap / "test" / "data" / "gate" / "tech_mapper";

  GNet *getNetForTechMap(const std::string &infile) {
    const std::filesystem::path subCatalog = "test/data/gate/parser/verilog";
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path prefixPath = homePath / subCatalog;

    std::string filename = prefixPath / (infile + ".v");

    text_diagnostics consumer;
    diagnostic_engine diag(&consumer);

    GateVerilogParser parser(infile);

    return_code result = read_verilog(filename, parser, &diag);
    EXPECT_EQ(result, return_code::success);

    return parser.getGnet();
  }

  std::string netPath(const std::string &nameDir) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath =
            homePath / "test/data/gate/tech_map/output" / nameDir;
    system(std::string("mkdir -p ").append(outputPath).c_str());
    system(std::string("mkdir -p ").append(outputPath / "before").c_str());
    return outputPath;
  }

  TEST(TechMapTest, gnet1) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    const std::string path = getenv("UTOPIA_HOME");

    LibraryCells libraryCells(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");

    SQLiteRWDatabase arwdb;
    std::string dbPath = "rwtest.db";

    arwdb.linkDB(dbPath);
    arwdb.openDB();

    initializeLibraryRwDatabase(libraryCells.cells, &arwdb);


    GNet net;
    gnet1(net);

    std::cout << "  Before tech map" << std::endl;
    std::cout << "N=" << net.nGates() << std::endl;
    std::cout << "I=" << net.nSourceLinks() << std::endl;

    std::cout << "  Tech maping" << std::endl;
    techMapPrinter(&net, 6, arwdb, SimpleTechMapper(), ReplacementVisitor(), netPath("gnet1"));

    std::cout << "  After tech map" << std::endl; 
    std::cout << "N=" << net.nGates() << std::endl;
    std::cout << "I=" << net.nSourceLinks() << std::endl;

    arwdb.closeDB();
    remove(dbPath.c_str());
  }

  TEST(TechMapTest, gnet2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    const std::string path = getenv("UTOPIA_HOME");

    LibraryCells libraryCells(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");

    SQLiteRWDatabase arwdb;
    std::string dbPath = "rwtest.db";

    arwdb.linkDB(dbPath);
    arwdb.openDB();

    initializeLibraryRwDatabase(libraryCells.cells, &arwdb);


    GNet net;
    gnet2(net);

    std::cout << "  Before tech map" << std::endl;
    std::cout << "N=" << net.nGates() << std::endl;
    std::cout << "I=" << net.nSourceLinks() << std::endl;

    std::cout << "  Tech maping" << std::endl;
    techMapPrinter(&net, 6, arwdb, SimpleTechMapper(), ReplacementVisitor(), netPath("gnet2"));

    std::cout << "  After tech map" << std::endl; 
    std::cout << "N=" << net.nGates() << std::endl;
    std::cout << "I=" << net.nSourceLinks() << std::endl;

    arwdb.closeDB();
    remove(dbPath.c_str());
  }

  TEST(TechMapTest, gnet3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    const std::string path = getenv("UTOPIA_HOME");

    LibraryCells libraryCells(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");

    SQLiteRWDatabase arwdb;
    std::string dbPath = "rwtest.db";

    arwdb.linkDB(dbPath);
    arwdb.openDB();

    initializeLibraryRwDatabase(libraryCells.cells, &arwdb);


    GNet net;
    gnet3(net);

    std::cout << "  Before tech map" << std::endl;
    std::cout << "N=" << net.nGates() << std::endl;
    std::cout << "I=" << net.nSourceLinks() << std::endl;

    std::cout << "  Tech maping" << std::endl;
    techMapPrinter(&net, 6, arwdb, SimpleTechMapper(), ReplacementVisitor(), netPath("gnet3"));

    std::cout << "  After tech map" << std::endl; 
    std::cout << "N=" << net.nGates() << std::endl;
    std::cout << "I=" << net.nSourceLinks() << std::endl;

    arwdb.closeDB();
    remove(dbPath.c_str());
  }

  TEST(TechMapTest, adder) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    const std::string path = getenv("UTOPIA_HOME");

    LibraryCells libraryCells(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");

    SQLiteRWDatabase arwdb;
    std::string dbPath = "rwtest.db";

    arwdb.linkDB(dbPath);
    arwdb.openDB();

    initializeLibraryRwDatabase(libraryCells.cells, &arwdb);

    GNet *net = getNetForTechMap("adder");

    std::cout << "  Before tech map" << std::endl;
    std::cout << "N=" << net->nGates() << std::endl;
    std::cout << "I=" << net->nSourceLinks() << std::endl;

    std::shared_ptr<GNet> sharedNet(net);

    // Premapping
    GateIdMap gmap;

    sharedNet->sortTopologically();
    std::shared_ptr<GNet> premapped = premap(sharedNet, gmap, PreBasis::AIG);
    
    GNet *gnet = premapped.get();

    std::cout << "  Tech maping" << std::endl;
    techMapPrinter(gnet, 6, arwdb, SimpleTechMapper(), ReplacementVisitor(), netPath("adder"));

    std::cout << "  After tech map" << std::endl;
    std::cout << std::endl;
    std::cout << "N=" << premapped->nGates() << std::endl;
    std::cout << "I=" << premapped->nSourceLinks() << std::endl;

    arwdb.closeDB();
    remove(dbPath.c_str());
  }

  TEST(TechMapTest, c17) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    const std::string path = getenv("UTOPIA_HOME");

    LibraryCells libraryCells(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");

    SQLiteRWDatabase arwdb;
    std::string dbPath = "rwtest.db";

    arwdb.linkDB(dbPath);
    arwdb.openDB();

    initializeLibraryRwDatabase(libraryCells.cells, &arwdb);

    GNet *net = getNetForTechMap("c17");

    std::cout << "  Before tech map" << std::endl;
    std::cout << "N=" << net->nGates() << std::endl;
    std::cout << "I=" << net->nSourceLinks() << std::endl;

    std::shared_ptr<GNet> sharedNet(net);

    // Premapping
    GateIdMap gmap;

    sharedNet->sortTopologically();
    std::shared_ptr<GNet> premapped = premap(sharedNet, gmap, PreBasis::AIG);
    
    GNet *gnet = premapped.get();

    std::cout << "  Tech maping" << std::endl;
    techMapPrinter(gnet, 6, arwdb, SimpleTechMapper(), ReplacementVisitor(), netPath("c17"));

    std::cout << "  After tech map" << std::endl;
    std::cout << std::endl;
    std::cout << "N=" << premapped->nGates() << std::endl;
    std::cout << "I=" << premapped->nSourceLinks() << std::endl;

    arwdb.closeDB();
    remove(dbPath.c_str());
  }

  TEST(TechMapTest, c432) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    const std::string path = getenv("UTOPIA_HOME");

    LibraryCells libraryCells(libertyDirrectTechMap.string() + "/sky130_fd_sc_hd__ff_n40C_1v95.lib");

    SQLiteRWDatabase arwdb;
    std::string dbPath = "rwtest.db";

    arwdb.linkDB(dbPath);
    arwdb.openDB();

    initializeLibraryRwDatabase(libraryCells.cells, &arwdb);

    GNet *net = getNetForTechMap("c432");

    std::cout << "  Before tech map" << std::endl;
    std::cout << "N=" << net->nGates() << std::endl;
    std::cout << "I=" << net->nSourceLinks() << std::endl;

    std::shared_ptr<GNet> sharedNet(net);

    // Premapping
    GateIdMap gmap;

    sharedNet->sortTopologically();
    std::shared_ptr<GNet> premapped = premap(sharedNet, gmap, PreBasis::AIG);
    
    GNet *gnet = premapped.get();

    std::cout << "  Tech maping" << std::endl;
    techMapPrinter(gnet, 6, arwdb, SimpleTechMapper(), ReplacementVisitor(), netPath("c432"));

    std::cout << "  After tech map" << std::endl;
    std::cout << std::endl;
    std::cout << "N=" << premapped->nGates() << std::endl;
    std::cout << "I=" << premapped->nSourceLinks() << std::endl;

    arwdb.closeDB();
    remove(dbPath.c_str());
  }

}
