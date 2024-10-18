//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
 
#include "gate/techmapper/subnet_unmapper.h"

#include <cassert>
#include <unordered_map>

namespace eda::gate::techmapper {

SubnetUnmapper::SubnetBuilderPtr SubnetUnmapper::map(
    const SubnetBuilderPtr &builder) const {
  auto newBuilder = std::make_shared<model::SubnetBuilder>();

  // Maps old links (w/o invertors) to new ones.
  std::unordered_map<model::Subnet::Link, model::Subnet::Link> links;

  for (auto it = builder->begin(); it != builder->end(); it.nextCell()) {
    const auto entryID = *it;
    const auto &oldCell = builder->getCell(entryID);

    model::Subnet::LinkList newInLinks(oldCell.getInNum());
    for (size_t i = 0; i < oldCell.getInNum(); ++i) {
      const auto &link = builder->getLink(entryID, i);
      const auto oldIdx = link.idx;
      const auto oldOut = static_cast<uint8_t>(link.out);
      const model::Subnet::Link oldInLink{oldIdx, oldOut, 0};

      const auto found = links.find(oldInLink);
      assert(found != links.end());

      const auto newIdx = found->second.idx;
      const auto newOut = static_cast<uint8_t>(found->second.out);
      const auto newInv = found->second.inv != link.inv;
      newInLinks[i] = model::Subnet::Link{newIdx, newOut, newInv};
    }

    // Recursive inlining.
    const auto newOutLinks = newBuilder->addCellRecursively(
        oldCell.getTypeID(), newInLinks, [](auto) { return true; });
    assert(oldCell.isOut() || newOutLinks.size() == oldCell.getOutNum());

    for (size_t i = 0; i < oldCell.getOutNum(); ++i) {
      const auto oldOut = static_cast<uint8_t>(i);
      const model::Subnet::Link oldOutLink{entryID, oldOut, 0};

      const auto &newOutLink = newOutLinks[i];
      links.emplace(oldOutLink, newOutLink);
    }
  }

  return newBuilder;
}

} // namespace eda::gate::techmapper
