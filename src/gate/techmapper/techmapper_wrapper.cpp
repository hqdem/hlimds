#include "gate/model2/printer/printer.h"
#include "gate/parser/graphml_to_subnet.h"
#include "gate/techmapper/techmapper_wrapper.h"

namespace eda::gate::tech_optimizer {
  using Subnet     = eda::gate::model::Subnet;
  using SubnetID   = eda::gate::model::SubnetID;

  void printVerilog(SubnetID subnet, std::string fileName) {
    eda::gate::model::ModelPrinter& verilogPrinter =
        eda::gate::model::ModelPrinter::getPrinter(eda::gate::model::ModelPrinter::VERILOG);
    std::ofstream outFile(fileName);
    verilogPrinter.print(outFile,
                        model::Subnet::get(subnet),
                        "techmappedNet");
    outFile.close();
  }
  int techMap(TechMapConfig config){

    std::string name = config.files[0];
    //silly check that file exists
    std::ifstream check(name);
    if(!check){
      check.close();
      std::cerr << "File "<< name << "is not found" << std::endl;
      return -1;
    }
    check.close();

    const std::string libertyPath = std::string(getenv("UTOPIA_HOME"))
                                  + "/test/data/gate/techmapper";
    SDC sdc{100000000, 10000000000};
    const auto& mapperType = config.type;

    Techmapper techmapper(libertyPath + "/sky130_fd_sc_hd__ff_100C_1v65.lib",
                          mapperType, sdc);
    std::cout <<"Start to process "<< name << std::endl;
    eda::gate::parser::graphml::GraphMlSubnetParser parser;
    
    SubnetID subnetID = parser.parse(name);
    SubnetID mappedSubnetID = techmapper.techmap(subnetID);
    printVerilog(mappedSubnetID, config.outNetFileName);
    return 0;
  };

} // namespace  eda::gate::tech_optimizer