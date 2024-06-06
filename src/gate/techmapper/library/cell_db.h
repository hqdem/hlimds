//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/celltype.h"
#include "gate/model/subnet.h"
#include "gate/techmapper/library/sc.h"
#include "gate/techmapper/library/subnetattr.h"

#include <kitty/print.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

namespace eda::gate::techmapper {

struct DTTHash {
  std::size_t operator()(const kitty::dynamic_truth_table& dtt) const {
    std::size_t hash = 0;
    for (auto block : dtt) {
      hash ^= std::hash<uint64_t>{}(block) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
  }
};

struct DTTEqual {
  bool operator()(const kitty::dynamic_truth_table& lhs, const kitty::dynamic_truth_table& rhs) const {
    return kitty::equal(lhs, rhs);
  }
};

class CellDB final {
  using CellTypeID = eda::gate::model::CellTypeID;
  using SubnetID = eda::gate::model::SubnetID;

public:
  CellDB(const std::vector<CellTypeID> &combCellTypeIDs,
         const SC::StandardSeqMap &seqCellTypeIDs);

  std::vector<SubnetID> &getPatterns();

  std::vector<SubnetID> getSubnetIDsByTT(const kitty::dynamic_truth_table &tt) const;
  const Subnetattr &getSubnetAttrBySubnetID(const SubnetID id) const;

private:
  std::vector<SubnetID> subnets;
  std::vector<SubnetID> patterns;

  std::unordered_map<kitty::dynamic_truth_table,  std::vector<SubnetID>, DTTHash, DTTEqual> ttSubnet;
  std::unordered_map<SubnetID, Subnetattr> subnetToAttr;

  bool isFunctionallyComplete();
};
} // namespace eda::gate::techmapper
