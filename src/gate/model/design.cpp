//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/design.h"

#include <cassert>
#include <unordered_set>

namespace eda::gate::model {

using LinkSet = std::unordered_set<Link>;
using LinkMap = NetDecomposer::LinkMap;

static void replace(const CellID oldCellID, const CellID newCellID,
                    const std::vector<uint16_t> &newInputs,
                    const std::vector<uint16_t> &newOutputs,
                    LinkMap &linkMap) {
  assert(oldCellID != newCellID);

  LinkSet removeSet;
  LinkMap insertMap;

  for (const auto &[oldLink, idx] : linkMap) {
    const auto &oldSource = oldLink.source;
    const auto &oldTarget = oldLink.target;

    const auto isOldSource = (oldSource.getCellID() == oldCellID);
    const auto isOldTarget = (oldTarget.getCellID() == oldCellID);

    if (isOldSource || isOldTarget) {
      LinkEnd newSource{newCellID, newOutputs[oldSource.getPort()]};
      LinkEnd newTarget{newCellID, newInputs[oldTarget.getPort()]};

      Link newLink{(isOldSource ? newSource : oldSource),
                   (isOldTarget ? newTarget : oldTarget)};
                   
      removeSet.insert(oldLink);
      insertMap.insert({newLink, idx});
    }
  }

  for (const auto oldLink : removeSet) {
    linkMap.erase(oldLink);
  }

  linkMap.insert(insertMap.begin(), insertMap.end());
}

void DesignBuilder::replaceCell(const CellID oldCellID, const CellID newCellID,
                                const std::vector<uint16_t> &newInputs,
                                const std::vector<uint16_t> &newOutputs) {
  for (size_t i = 0; i < mapping.size(); ++i) {
    replace(oldCellID, newCellID, newInputs, newOutputs, mapping[i].inputs);
    replace(oldCellID, newCellID, newInputs, newOutputs, mapping[i].outputs);
  }
}

} // namespace eda::gate::model
