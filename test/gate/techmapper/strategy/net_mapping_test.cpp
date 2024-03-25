#include "gtest/gtest.h"
#include "gate/techmapper/techmapper_test_util.h"
#include "gate/techmapper/techmapper.h"
#include "gate/techmapper/util/get_tech_attr.h"

namespace eda::gate::tech_optimizer {

auto netMapperType = Techmapper::MapperType::SIMPLE_AREA_FUNC;

TEST(TechMapTest, SimpleNet) {
  NetID mappedSubnetId = simpleNetMapping(netMapperType);
}

} // namespace eda::gate::tech_optimizer