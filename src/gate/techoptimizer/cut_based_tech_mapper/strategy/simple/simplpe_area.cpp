
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/cone_builder.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/simple/bestSimpleReplacement.h"
#include "gate/techoptimizer/cut_based_tech_mapper/strategy/simple/simplpe_area.h"

using Subnet = eda::gate::model::Subnet;
namespace eda::gate::tech_optimizer {

float calculateArea(const std::unordered_set<uint64_t> &entryIdxs, const SubnetID &subID,
                     const std::map<EntryIndex, BestReplacement> &bestReplacementMap,
                     const CellDB &cellDb) {
  float area = 0;
  Subnet &subnet = Subnet::get(subID);
  eda::gate::model::Array<Subnet::Entry> entries = subnet.getEntries();

  std::stack<uint64_t> stack;
  std::unordered_set<uint64_t> visited;

  for (const auto& out : entryIdxs) {
    stack.push(out);
    visited.insert(out);
  }

  while (!stack.empty()) {
    EntryIndex currentEntryIDX = stack.top();
    auto currentCell = entries[currentEntryIDX].cell;
    if ((!currentCell.isIn() || currentCell.getSymbol() != model::CellSymbol::IN)
        && (!currentCell.isOut() || currentCell.getSymbol() != model::CellSymbol::OUT)) {
      area += cellDb.getSubnetAttrBySubnetID(bestReplacementMap.at(
          currentEntryIDX).subnetID).area;
    }
    stack.pop();
    for (const auto &link : currentCell.link) {
      if (visited.find(link.idx) == visited.end()) {
        stack.push(link.idx);
        visited.insert(link.idx);
      }
    }
  }
  return area;
}

void SimplifiedStrategy::findBest(EntryIndex entryIndex, const CutsList &cutsList,
                                  std::map<EntryIndex, BestReplacement> &bestReplacementMap,
                                  CellDB &cellDB,
                                  SubnetID subnetID) {
  eda::gate::optimizer2::ConeBuilder coneBuilder(&Subnet::get(subnetID));
  BestReplacement bestSimpleReplacement{};
  float bestArea = MAXFLOAT;
  // Iterate over all cuts to find the best replacement
  for (const auto &cut : cutsList) {
    if (cut.entryIdxs.size() != 1) {

      SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;

      auto truthTable = eda::gate::model::evaluate(
          model::Subnet::get(coneSubnetID));

      for (const SubnetID &currentSubnetID : cellDB.getSubnetIDsByTT(truthTable)) {
        auto currentAttr = cellDB.getSubnetAttrBySubnetID(currentSubnetID);

        float area = calculateArea(cut.entryIdxs, subnetID, bestReplacementMap, cellDB)
            + currentAttr.area;

        if (area < bestArea) {
          bestArea = area;
          bestSimpleReplacement.subnetID = currentSubnetID;
          bestSimpleReplacement.entryIDxs = cut.entryIdxs;
        }
      }
    }
  }
  bestReplacementMap[entryIndex] = bestSimpleReplacement;
}
} // namespace eda::gate::tech_optimizer