//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/cut_walker.h"

namespace eda::gate::optimizer {

  CutWalker::CutWalker(GNet *gNet, CutVisitor *visitor, CutStorage *cutStorage)
          : Walker(gNet, nullptr), cutStorage(cutStorage), cutVisitor(visitor) {
  }

  VisitorFlags CutWalker::callVisitor(Walker::GateId node) {
    auto flag = cutVisitor->onNodeBegin(node);

    if (flag != CONTINUE) {
      return flag;
    }

    if (cutStorage) {
      auto &cuts = cutStorage->cuts[node];
      for (const auto &cut: cuts) {

        flag = cutVisitor->onCut(node, cut);

        // Only CONTINUE, FINISH_ALL_NODES and SKIP are expected.
        switch (flag) {
          case CONTINUE:
          case FINISH_FURTHER_NODES:
            break;
          case FINISH_ALL_NODES:
          case SKIP:
            return flag;
          default:
            std::cerr << "Unexpected flag in CutWalker CutVisitor::onCut."
                      << std::endl;
        }
      }
    }

    return cutVisitor->onNodeEnd(node);
  }

} // namespace eda::gate::optimizer
