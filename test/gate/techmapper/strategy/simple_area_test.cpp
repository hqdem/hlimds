#include "gtest/gtest.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/techmapper.h"
#include "gate/techmapper/util/get_tech_attr.h"
#include "gate/techmapper/util/get_statistic.h"

namespace eda::gate::tech_optimizer {

auto areaMapperType = Techmapper::MapperType::SIMPLE_AREA_FUNC;
std::string libertyName = "test/data/gate/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib";

TEST(TechmapAreaTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(areaMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
  printStatistic(mappedSubnetId, libertyName);
}

TEST(TechmapAreaTest, SimpleAndNotSubnet) {
  SubnetID mappedSubnetId = andNotMapping(areaMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
  printStatistic(mappedSubnetId, libertyName);
}

TEST(TechmapAreaTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(areaMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
  printStatistic(mappedSubnetId, libertyName);}

TEST(TechmapAreaTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(areaMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
  printStatistic(mappedSubnetId, libertyName);}

TEST(TechmapAreaTest, GraphMLSubnet) {
  std::string fileName = "simple_spi_orig";
  SubnetID mappedSubnetId = graphMLMapping(areaMapperType, fileName);
  std::cout << getArea(mappedSubnetId) << std::endl;
  printStatistic(mappedSubnetId, libertyName);}

TEST(TechmapAreaTest, DISABLED_GraphMLSubnet) {
  std::string fileName = "aes_orig";
  SubnetID mappedSubnetId = graphMLMapping(areaMapperType, fileName);
  std::cout << getArea(mappedSubnetId) << std::endl;
  printStatistic(mappedSubnetId, libertyName);}

TEST(TechmapAreaTest, DISABLED_RandomSubnet) {
  SubnetID mappedSubnetId = randomMapping(areaMapperType);
  std::cout << getArea(mappedSubnetId) << std::endl;
  printStatistic(mappedSubnetId, libertyName);}

} // namespace eda::gate::tech_optimizer