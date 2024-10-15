//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnetview.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/techmapper/matcher/pbool_matcher.h"

#include "gate/debugger/sat_checker.h"
#include "gate/model/utils/subnet_checking.h"
#include "gate/optimizer/synthesis/isop.h"

namespace eda::gate::techmapper {
using StandardCell = library::StandardCell;

std::vector<SubnetTechMapperBase::Match> PBoolMatcher::match(
    const util::TruthTable &truthTable,
    const std::vector<model::EntryID> &entryIdxs) {
  std::vector<SubnetTechMapperBase::Match> matches;

  auto config = kitty::exact_p_canonization(truthTable);
  const auto &ctt = util::getTT(config); // canonized TT
  util::NpnTransformation t = util::getTransformation(config);
  std::vector<std::pair<StandardCell, uint16_t>> scs;

  match(scs, ctt);

  for (const auto &cell : scs) {
    const auto output = cell.second;
    const auto& techCell = cell.first;

    assert(techCell.ctt[output] == ctt);

    model::Subnet::LinkList linkList(
        techCell.transform[output].permutation.size());
    uint i = 0;
    for (const auto &index : t.permutation) {
      linkList[techCell.transform[output].permutation.at(i++)] =
        model::Subnet::Link{(uint32_t)entryIdxs.at(index)};
    }

    const auto match = SubnetTechMapperBase::Match{
      techCell.cellTypeID, linkList, output};
    matches.push_back(match);

#ifdef DEBUG_MOUTS
    const auto synthesizer = optimizer::synthesis::MMSynthesizer();
    auto subnetObject = synthesizer.synthesize(truthTable);
    const auto beforeID = subnetObject.make();

    model::SubnetBuilder builder;
    model::Subnet::LinkList inputs;
    for (uint i = 0; i < t.permutation.size(); i++) {
      const auto input = builder.addInput();
      inputs.push_back(input);
    }

    model::Subnet::LinkList inputsToCheck(
      techCell.transform[output].permutation.size());
    i = 0;
    for (const auto &index : t.permutation) {
      inputsToCheck[techCell.transform[output].permutation.at(i++)] =
        model::Subnet::Link{inputs.at(index)};
    }

    const auto cellToCheck = builder.addSubnet(
      model::CellType::get(techCell.cellTypeID).getSubnet(), inputsToCheck);
    builder.addOutput(cellToCheck[output]);

    const auto subnetID = builder.make();

    try {
      const auto ttToCheck = model::evaluate(model::Subnet::get(subnetID));
      if (truthTable != ttToCheck[0]) {
        std::cout << "Truth table equivalence check failed:\n";
        std::cout << "requested truthTable=" << kitty::to_hex(truthTable) << std::endl;
        std::cout << "correspondent subnet=\n" << model::Subnet::get(beforeID) << std::endl;
        std::cout << "constructed subnet=\n" << model::Subnet::get(subnetID) << std::endl;
        std::cout << "its truth table=" << kitty::to_hex(ttToCheck[0]) << std::endl;
        std::cout << "but as the canonized truth table we keep " << kitty::to_hex(techCell.ctt[output]) << " on output " << output << std::endl;
      }
    } catch (const std::exception &exp) {
      std::cout << exp.what() << std::endl;
    }

    debugger::SatChecker &checker = debugger::SatChecker::get();
    const auto checkerResult = checker.areEquivalent(beforeID, subnetID);
    if(!checkerResult.equal()) {
      std::cout <<"###################################################################################\n";
      std::cout << techCell.name << "\n";
      std::cout <<"###################################################################################\n";
      std::cout << "Subnet equivalence check failed:\n";
      auto tt2 = model::evaluate(model::Subnet::get(subnetID));
      std::cout << model::Subnet::get(beforeID) << "tt=" << kitty::to_hex(truthTable) << std::endl << std::endl;
      std::cout << model::Subnet::get(subnetID) << "tt2[0]=" << kitty::to_hex(tt2[0]) << std::endl;
      std::cout << "requested truthTable=" << kitty::to_hex(truthTable) << std::endl;
      std::cout << "canonized truthTable=" << kitty::to_hex(ctt) << "; perm vector: "; for (const auto perm : t.permutation) std::cout << (int)(perm) << " "; std::cout << std::endl;
      std::cout << "canonized cell truthTable=" << kitty::to_hex(techCell.ctt[output]) << "; perm vector: "; for (const auto perm : techCell.transform[output].permutation) std::cout << (int)(perm) << " "; std::cout << std::endl;
      std::cout << "requested subnet:\n" << model::Subnet::get(beforeID) << std::endl;
      std::cout << "found subnet:\n" << model::Subnet::get(subnetID) << std::endl;
      std::cout << "output number: " << output << std::endl;
      std::cout << "links in subnet: "; for (const auto entry : entryIdxs) std::cout << (int) entry << " "; std::cout << std::endl;
      std::cout << "links to be connected: "; for (const auto input : linkList) std::cout << (int) input.idx << " "; std::cout << std::endl;
      const auto ce = checkerResult.getCounterExample();
      std::cout << "counter example: " << std::endl; for (const auto c : ce) std::cout << (int) c; std::cout << std::endl;
      assert(false);
    }
#endif
  }
  return matches;
}

std::vector<SubnetTechMapperBase::Match> PBoolMatcher::match(
    const std::shared_ptr<model::SubnetBuilder> &builder,
    const optimizer::Cut &cut) {
  if (cut.isTrivial()) {
    const auto truthTable = util::getZeroTruthTable<util::TruthTable>(0);
    const auto isZero = builder->getCell(cut.rootID).isZero();
    return match(isZero ? truthTable : ~truthTable, {});
  }

  const model::SubnetView cone(builder, cut);
  const auto truthTable = cone.evaluateTruthTable();
  const std::vector<model::EntryID> entryIdxs(
      cut.leafIDs.begin(), cut.leafIDs.end());

  return match(truthTable, entryIdxs);
}

} // namespace eda::gate::techmapper
