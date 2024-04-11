//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/printer/printer.h"
#include "gate/parser/gate_verilog.h"
#include "gate/parser/graphml_to_subnet.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/util/get_tech_attr.h"
#include "gtest/gtest.h"

#include <filesystem>

using Subnet     = eda::gate::model::Subnet;
using SubnetID   = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

const std::string libertyPath = std::string(getenv("UTOPIA_HOME"))
                                 + "/test/data/gate/techmapper";

void printVerilog(SubnetID subnet, std::string fileName) {
  eda::gate::model::ModelPrinter& verilogPrinter =
      eda::gate::model::ModelPrinter::getPrinter(eda::gate::model::ModelPrinter::VERILOG);
  std::ofstream outFile("test/data/gate/techmapper/print/"+ fileName);
  verilogPrinter.print(outFile,
                       model::Subnet::get(subnet),
                       "techmappedNet");
  outFile.close();
}

SubnetID parseGraphML(std::string fileName);
bool checkAllCellsMapped(SubnetID subnetID);

std::vector<std::string> names = {
  "i2c_orig",
  "sasc_orig",
  "simple_spi_orig",
  "usb_phy_orig",
  "ac97_ctrl_orig",
  "wb_dma_orig",
  "tv80_orig", // 12k
  "ss_pcm_orig", // 1k
  "spi_orig", // 5k
  "sha256_orig", // 19k
  "pci_orig", // 25k
  "mem_ctrl_orig", //20k 
  "iir_orig", // 8k
  // "fpu_orig", // 30k
  "fir_orig",// 5k
  "dynamic_node_orig", // 23k
  "des3_area_orig", // 5k
  "aes_orig",// 30k

  // "picosoc_orig", //105k 
  // "tinyRocket_orig", //60k
  // "vga_lcd_orig", // ~140k nodes 
  // "wb_conmax_orig", // ~50k nodes

};

void testMapper(Techmapper::MapperType mapperType, std::string suff){
  SDC sdc{100000000, 10000000000};
  
  Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib",
                        mapperType, sdc);

  for(auto &name : names){
    auto start = std::chrono::high_resolution_clock::now();

    std::cout <<"Start to process "<< name << std::endl;
    SubnetID subnetID = parseGraphML(name);
    SubnetID mappedSubnetID = techmapper.techmap(subnetID);
    EXPECT_TRUE(checkAllCellsMapped(mappedSubnetID));
    printVerilog(mappedSubnetID, name + "_" + suff);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> FindBestTime = end - start;
    std::cout <<"end "<< name <<": "<< FindBestTime.count() << " seconds.\n";
  }
}


TEST(TechMapTest, graphML_Power_group) {
  std::string suff = "powerMapped.v";
  testMapper(Techmapper::MapperType::POWER,suff);
}

TEST(TechMapTest, graphML_SimpleArea_group) {
  std::string suff = "simpleAreaMapped.v";
  testMapper(Techmapper::MapperType::SIMPLE_AREA_FUNC,suff);
}


// TEST(TechMapTest, graphML_Delay_group) {
//   std::string suff = "delayMapped.v";
//   testMapper(Techmapper::MapperType::DELAY,suff);
// }

// TEST(TechMapTest, graphML_AF_group) {
//   std::string suff = "afMapped.v";
//   testMapper(Techmapper::MapperType::AREA_FLOW,suff);
// }

} // namespace eda::gate::tech_optimizer
