#include "gtest/gtest.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techoptimizer/techoptimizer.h"
#include "gate/techoptimizer/util/get_tech_attr.h"

namespace eda::gate::tech_optimizer {

auto delayMapperType = Techmapper::MapperType::SIMPLE_DELAY_FUNC;

TEST(TechmapDelayTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(delayMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapDelayTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(delayMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapDelayTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(delayMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapDelayTest, DISABLED_GraphMLSubnet) {
  std::string fileName = "aes_orig";
  SubnetID mappedSubnetId = graphMLMapping(delayMapperType, fileName);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

TEST(TechmapDelayTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(delayMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
}

} // namespace eda::gate::tech_optimizer