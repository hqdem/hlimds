#include "gate/techmapper/techmapper.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/util/get_statistic.h"
#include "gate/techmapper/util/get_tech_attr.h"
#include "gtest/gtest.h"

namespace eda::gate::tech_optimizer {

auto areaRecoveryMapperType = Techmapper::MapperType::AREA_FLOW;
std::string testLibertyName =
    "test/data/gate/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib";

TEST(TechmapAreaRecoveryTest, SubnetFromPaper) {
  SubnetID mappedSubnetId = areaRecoveySubnetMapping(areaRecoveryMapperType);
  printStatistic(mappedSubnetId, testLibertyName);
}

TEST(TechmapAreaRecoveryTest, GraphMLSubnet) {
  std::string fileName = "ss_pcm_orig";
  SubnetID mappedSubnetId = graphMLMapping(areaRecoveryMapperType, fileName);
  printStatistic(mappedSubnetId, testLibertyName);
}

TEST(TechmapAreaRecoveryTest, SimpleAndSubnet) {
  SubnetID mappedSubnetId = simpleANDMapping(areaRecoveryMapperType);
  printStatistic(mappedSubnetId, testLibertyName);
}

TEST(TechmapAreaRecoveryTest, SimpleORSubnet) {
  SubnetID mappedSubnetId = simpleORMapping(areaRecoveryMapperType);
  printStatistic(mappedSubnetId, testLibertyName);
}

TEST(TechmapAreaRecoveryTest, SimpleSubnet) {
  SubnetID mappedSubnetId = andNotMapping(areaRecoveryMapperType);
  printStatistic(mappedSubnetId, testLibertyName);
}

} // namespace eda::gate::tech_optimizer