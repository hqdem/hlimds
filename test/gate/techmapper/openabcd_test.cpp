//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/printer/printer.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/utils/get_statistics.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

namespace eda::gate::techmapper {

using Subnet        = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID      = model::SubnetID;
using ModelPrinter  = model::ModelPrinter;

void printVerilog(const SubnetID subnet, std::string &fileName) {
  ModelPrinter& verilogPrinter =
    ModelPrinter::getPrinter(ModelPrinter::VERILOG);
  std::ofstream outFile("test/data/gate/techmapper/print/" + fileName);
  verilogPrinter.print(outFile,
                       model::Subnet::get(subnet),
                       "techmappedNet");
  std::cout << "Output Verilog file: " <<
    "test/data/gate/techmapper/print/" + fileName << std::endl;
  outFile.close();
}

SubnetID parseGraphML(const std::string &fileName);
bool checkAllCellsMapped(SubnetID subnetID);

std::vector<std::string> names = {
  "i2c_orig",          // <1k
  "sasc_orig",         // <1k
  "simple_spi_orig",   // <1k
  "usb_phy_orig",      // <1k
  "ss_pcm_orig",       //  1k
  "wb_dma_orig",       //  3k
  "des3_area_orig",    //  5k
  "fir_orig",          //  5k
  "spi_orig",          //  5k
  "iir_orig",          //  8k
  "ac97_ctrl_orig",    // 10k
  "tv80_orig",         // 12k
  "sha256_orig",       // 19k
  "dynamic_node_orig", // 23k
  "pci_orig",          // 25k
  "mem_ctrl_orig",     // 20k
  "aes_orig",          // 30k
  "fpu_orig",          // 30k
  "wb_conmax_orig",    // 50k
  "tinyRocket_orig",   // 60k
  "picosoc_orig",     // 105k
  "vga_lcd_orig"      // 140k
};

void testMapper(const Techmapper::Strategy mapperType,
                std::string &suff) {
  Techmapper techmapper;

  techmapper.setStrategy(mapperType);
  techmapper.setSDC(sdcPath);
  techmapper.setLibrary(techLib);

  for(auto &name : names) {
    auto start = std::chrono::high_resolution_clock::now();

    std::cout << std::endl << "Start to techmap " <<
      name + ".bench.graphml" << std::endl;

    const auto subnetId = parseGraphML(name);
    SubnetBuilder builder;
    techmapper.techmap(subnetId, builder);
    const auto mappedSubnetID = builder.make();
    EXPECT_TRUE(checkAllCellsMapped(mappedSubnetID));

    std::string fileName(name + "_" + suff);
    printVerilog(mappedSubnetID, fileName);

    auto end = std::chrono::high_resolution_clock::now();
    auto time = end - start;

    printStatistics(mappedSubnetID, time);
  }
}

TEST(TechmapTest, graphML_Power_group) {
  std::string suff = "power.v";
  testMapper(Techmapper::Strategy::POWER, suff);
}

TEST(TechmapTest, graphML_SimpleArea_group) {
  std::string suff = "simple_area.v";
  testMapper(Techmapper::Strategy::AREA, suff);
}

TEST(TechmapTest, graphML_Delay_group) {
  std::string suff = "delay.v";
  testMapper(Techmapper::Strategy::DELAY, suff);
}

TEST(TechmapTest, graphML_AF_group) {
  std::string suff = "af.v";
  testMapper(Techmapper::Strategy::AREA_FLOW, suff);
}

} // namespace eda::gate::techmapper
