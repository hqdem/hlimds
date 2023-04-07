//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cuts_finder_visitor.h"
#include "gate/optimizer/links_clean.h"
#include "gate/optimizer/util.h"
#include "gate/optimizer/visitor.h"
#include "gate/simulator/simulator.h"

#include <queue>

namespace eda::gate::optimizer {
/**
 * \brief Realization of interface Visitor.
 * \ Handler of the node and its cut to execute rewriting.
 * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
 */
  class OptimizerVisitor : public Visitor {
  public:

    using Order = std::vector<GateID>;

    struct SwapOption {
      GNet *subsNet = nullptr;
      Order order;

      SwapOption(GNet *subsNet, Order &order) : subsNet(subsNet), order(std::move(order)){
        std::cout << "Regular constructor " << std::endl;
      }

      SwapOption(SwapOption &&src)  noexcept {
        std::cout << "Move constructor " << std::endl;

        this->subsNet = src.subsNet;
        src.subsNet = nullptr;
        std::cout << "Move constructor is called for " << (subsNet ? subsNet->id() : 666) << std::endl;
      }

      ~SwapOption() {
        std::cout << "Destructor is called for " << (subsNet ? subsNet->id() : 6666) << std::endl;
        std::cout << "Order size: " << order.size() << "\n\n";
        delete subsNet;
      }
    };

    OptimizerVisitor();

    void set(CutStorage *cutStorage, GNet *net, int cutSize);

    VisitorFlags onNodeBegin(const GateID &) override;

    VisitorFlags onNodeEnd(const GateID &) override;

    VisitorFlags onCut(const Cut &) override;

  private:
    CutStorage *cutStorage;

    CutStorage::Cuts *lastCuts;
    std::vector<const CutStorage::Cut *> toRemove;

    bool checkValidCut(const Cut &cut);

  protected:
    GNet *net;
    GateID lastNode;
    int cutSize;

    virtual bool checkOptimize(const Cut &cut, const SwapOption &option,
                               const std::unordered_map<GateID, GateID> &map) = 0;

    /// учесть оптимизацию.
    virtual VisitorFlags
    considerOptimization(const Cut &cut, const SwapOption &option,
                         const std::unordered_map<GateID, GateID> &map) = 0;

    virtual std::vector<SwapOption> getSubnets(uint64_t func) = 0;

  };

} // namespace eda::gate::optimizer