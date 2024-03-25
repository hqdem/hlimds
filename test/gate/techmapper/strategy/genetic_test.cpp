#include "gtest/gtest.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/techmapper.h"
#include "gate/techmapper/util/get_tech_attr.h"

namespace eda::gate::tech_optimizer {

auto geneticMapperType = Techmapper::MapperType::GENETIC;

TEST(TechmapGeneticTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(geneticMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapGeneticTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(geneticMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapGeneticTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(geneticMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapGeneticTest, DISABLED_GraphMLSubnet) {
  std::string fileName = "aes_orig";
  SubnetID mappedSubnetId = graphMLMapping(geneticMapperType, fileName);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapGeneticTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(geneticMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

} // namespace eda::gate::tech_optimizer