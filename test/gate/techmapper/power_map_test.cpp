#include "gate/techoptimizer/power_map/power_map.h"
#include "gtest/gtest.h"

TEST(TechMapTest, switchFlow) {
    if (!getenv("UTOPIA_HOME")) {
        FAIL() << "UTOPIA_HOME is not set.";
    }
    eda::gate::tech_optimizer::switchFlowTest1();
}
