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

using Subnet       = eda::gate::model::Subnet;
using SubnetID     = eda::gate::model::SubnetID;
using ModelPrinter = eda::gate::model::ModelPrinter;

void printVerilog(SubnetID subnet, std::string fileName) {
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

SubnetID parseGraphML(std::string fileName);
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

void testMapper(Techmapper::MapperType mapperType, std::string suff) {
  SDC sdc{100000000, 10000000000};

  Techmapper techmapper(techLib, mapperType, sdc);

  for(auto &name : names) {
    auto start = std::chrono::high_resolution_clock::now();

    std::cout << std::endl << "Start to techmap " <<
      name + ".bench.graphml" << std::endl;

    SubnetID subnetId = parseGraphML(name);
    SubnetID mappedSubnetId = techmapper.techmap(subnetId);
    EXPECT_TRUE(checkAllCellsMapped(mappedSubnetId));

    printVerilog(mappedSubnetId, name + "_" + suff);

    auto end = std::chrono::high_resolution_clock::now();
    auto time = end - start;

    printStatistics(mappedSubnetId, time);
  }
}

TEST(TechmapTest, DISABLED_graphML_Power_group) {
  std::string suff = "power.v";
  testMapper(Techmapper::MapperType::POWER, suff);
}

TEST(TechmapTest, DISABLED_graphML_SimpleArea_group) {
  std::string suff = "simple_area.v";
  testMapper(Techmapper::MapperType::SIMPLE_AREA_FUNC, suff);
}

TEST(TechmapTest, DISABLED_graphML_Delay_group) {
  std::string suff = "delay.v";
  testMapper(Techmapper::MapperType::SIMPLE_DELAY_FUNC, suff);
}

TEST(TechmapTest, DISABLED_graphML_AF_group) {
  std::string suff = "af.v";
  testMapper(Techmapper::MapperType::AREA_FLOW, suff);
}

} // namespace eda::gate::techmapper
