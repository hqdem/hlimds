#include "gtest/gtest.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techoptimizer/techoptimizer.h"
#include "gate/techoptimizer/util/get_tech_attr.h"

namespace eda::gate::tech_optimizer {

auto areaMapperType = Techmapper::MapperType::SIMPLE_AREA_FUNC;

TEST(TechmapAreaTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(areaMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapAreaTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(areaMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapAreaTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(areaMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapAreaTest, DISABLED_GraphMLSubnet) {
  std::string fileName = "aes_orig";
  SubnetID mappedSubnetId = graphMLMapping(areaMapperType, fileName);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapAreaTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(areaMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

} // namespace eda::gate::tech_optimizer