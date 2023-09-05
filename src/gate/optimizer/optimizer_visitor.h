//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cone_visitor.h"
#include "gate/optimizer/cut_visitor.h"
#include "gate/optimizer/cuts_finder_visitor.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/optimizer/util.h"

#include <queue>

namespace eda::gate::optimizer {

 /**
  * \brief Handler of nodes and its cuts to execute rewriting.
  */
  class OptimizerVisitor : public CutVisitor {

  public:

    using BoundGNetList = BoundGNet::BoundGNetList;

    OptimizerVisitor();

    /**
     * Setter for the fields based on which optimization is performed.
     */
    void set(CutStorage *cutStorage, GNet *net, unsigned int cutSize,
             unsigned int maxCutsNumber);

    VisitorFlags onNodeBegin(const GateId &) override;

    VisitorFlags onNodeEnd(const GateId &) override;

    VisitorFlags onCut(const GateId &, const Cut &) override;

    /**
     * \brief Checks if substitution is optimizing.
     * @param lastNode Node, which cone substitution is examined.
     * @param option Net with which substitution is executed.
     * @param map Maps cone inputs and substitute net sources.
     * @return true if substitution is optimizing.
     */
    virtual bool checkOptimize(const GateId &lastNode, const BoundGNet &option,
                               MatchMap &map) = 0;

    /**
     * \brief Makes real changes in the net.
     * @param lastNode Node, which cone will be used for substitution.
     * @param option Net with which substitution is executed.
     * @param map Maps cone inputs and substitute net sources.
     */
    virtual void considerOptimization(const GateId &lastNode, BoundGNet &option,
                                      MatchMap &map) = 0;

    /**
     * Finishes making real changes in the net.
     * @param lastNode Node, which cone was used for substitution.
     */
    virtual VisitorFlags finishOptimization(const GateId &lastNode) {
      return CONTINUE;
    }

    /**
     * @param func Truth table for the function.
     * @return Struct with list of nets that implement such function.
     */
    virtual BoundGNetList getSubnets(uint64_t func) = 0;

  private:
    CutStorage *cutStorage;
    CutStorage::Cuts *lastCuts;
    std::vector<const CutStorage::Cut *> toRemove;
    unsigned int maxCutsNumber;

    bool checkValidCut(const GateId &lastNode, const Cut &cut);

  protected:
    GNet *net;
    unsigned int cutSize;

  };

} // namespace eda::gate::optimizer
