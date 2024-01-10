
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/cone_builder.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/simple/bestSimpleReplacement.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/simple/simplpe_area.h"

using Subnet = eda::gate::model::Subnet;
namespace eda::gate::tech_optimizer {

void SimplifiedStrategy::findBest(EntryIndex entryIndex, const CutsList &cutsList,
                                  std::map<EntryIndex, BestReplacement> &bestReplacementMap,
                                  CellDB &cellDB,
                                  SubnetID subnetID) {
  eda::gate::optimizer2::ConeBuilder coneBuilder(&Subnet::get(subnetID));
  BestSimpleReplacement bestSimpleReplacement{};
  float bestArea = 10000.0;

  // Iterate over all cuts to find the best replacement
  for (const auto &cut : cutsList) {
    if (cut.entryIdxs.size() != 1) {
      SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;
      std::cout << Subnet::get(coneSubnetID) << std::endl;

      auto truthTable = eda::gate::model::evaluate(
          model::Subnet::get(coneSubnetID));

      for (const SubnetID &currentSubnetID : cellDB.getSubnetIDsByTT(truthTable)) {
        auto currentAttr = cellDB.getSubnetAttrBySubnetID(currentSubnetID);

        float area = currentAttr->area;
        if (area < bestArea) {
          bestArea = area;
          bestSimpleReplacement.area = area;
          bestSimpleReplacement.subnetID = currentSubnetID;
          bestSimpleReplacement.entryIDxs = cut.entryIdxs;
        }
      }
    }
  }
  bestReplacementMap[entryIndex] = bestSimpleReplacement;
}
} // namespace eda::gate::tech_optimizer