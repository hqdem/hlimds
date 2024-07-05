//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/sdc_manager.h"
#include "gate/model/printer/printer.h"
#include "gate/parser/graphml_parser.h"
#include "gate/premapper/aigmapper.h"
#include "gate/techmapper/techmapper_wrapper.h"
#include "gate/techmapper/utils/get_statistics.h"
#include "util/env.h"

namespace eda::gate::techmapper {

  using ModelPrinter  = model::ModelPrinter;
  using Subnet        = model::Subnet;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetID      = model::SubnetID;

  void printVerilog(const SubnetID subnet, const std::string &fileName) {
    ModelPrinter& verilogPrinter =
        ModelPrinter::getPrinter(ModelPrinter::VERILOG);
    std::ofstream outFile(fileName);
    verilogPrinter.print(outFile, Subnet::get(subnet), "techmappedNet");
    outFile.close();
  }

  int techMap(const TechMapConfig config) {
    std::string name = config.files[0];
    std::ifstream check(name);
    if (!check) {
      check.close();
      std::cerr << "File " << name << " is not found!" << std::endl;
      return -1;
    }
    check.close();

    // TODO: it should be an option
    const std::filesystem::path techLib = eda::env::getHomePath() /
      "test/data/gate/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib";
    const std::filesystem::path sdcPath = eda::env::getHomePath() /
      "test/data/gate/techmapper/test.sdc";

    Techmapper techmapper;
    techmapper.setStrategy(config.strategy);
    techmapper.setSDC(sdcPath);
    techmapper.setLibrary(techLib);

    auto startTime = std::chrono::high_resolution_clock::now();

    std::cout << "Start to techmap " << name << std::endl;

    // Read input GraphML file.
    parser::graphml::GraphMlParser parser;
    const auto subnetID = parser.parse(name)->make();
    // Premap the input data into AIG.
    premapper::AigMapper aigMapper("aig");
    const auto premappedSubnetID = aigMapper.transform(subnetID);
    // Techmapping.
    SubnetBuilder builder;
    techmapper.techmap(premappedSubnetID, builder);
    const auto mappedSubnetID = builder.make();

    printVerilog(mappedSubnetID, config.outNetFileName);

    auto finishTime = std::chrono::high_resolution_clock::now();
    auto processingTime = finishTime - startTime;

    printStatistics(mappedSubnetID, processingTime);

    return 0;
  }
} // namespace eda::gate::techmapper
