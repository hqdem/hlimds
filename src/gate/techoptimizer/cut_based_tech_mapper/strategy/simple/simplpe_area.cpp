
#include "gate/optimizer2/cone_builder.cpp"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/simple/bestSimpleReplacement.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/simple/simplpe_area.h"
#include "gate/model2/utils/subnet_truth_table.h"

using Subnet = eda::gate::model::Subnet;
namespace eda::gate::tech_optimizer {

void SimplifiedStrategy::findBest(EntryIndex entryIndex, const CutsList &cutsList,
                                  std::map<EntryIndex, BestReplacement> &bestReplacementMap,
                                  CellDB &cellDB,
                                  SubnetID subnetID) {
  eda::gate::optimizer2::ConeBuilder coneBuilder(&Subnet::get(subnetID));
  BestSimpleReplacement bestSimpleReplacement{};

  // Iterate over all cuts to find the best replacement
  for (const auto &cut : cutsList) {
    auto truthTable = eda::gate::model::evaluate(
        model::Subnet::get(coneBuilder.getCone(cut).subnetID));

    for (const auto &currentSubnetID : cellDB.getSubnetIDsByTT(truthTable)) {
      auto currentAttr = cellDB.getSubnetAttrBySubnetID(currentSubnetID);
      if (!currentAttr.has_value()) continue; // Skip if no attribute found

      float currentArea = currentAttr->area;
      if (currentArea < bestSimpleReplacement.area) {
        bestSimpleReplacement.area = currentArea;
        bestSimpleReplacement.subnetID = currentSubnetID;
        bestSimpleReplacement.entryIDxs = cut.entryIdxs;
      }
    }
  }

  bestReplacementMap[entryIndex] = bestSimpleReplacement;
}
} // eda::gate::tech_optimizer