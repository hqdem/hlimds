//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/optimizer/cut_visitor.h"
#include "gate/optimizer/cuts_finder_visitor.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/optimizer/util.h"
#include "gate/tech_optimizer/cut_based_tech_mapper/replacement_struct.h"
#include "gate/tech_optimizer/cut_based_tech_mapper/strategy/strategy.h"
#include "gate/model2/celltype.h"

#include <queue>

namespace eda::gate::tech_optimizer {
/**
 * \brief Realization of interface Visitor.
 * \author <a href="mailto:dgaryaev@ispras.ru"></a>
 */

  class  SearchOptReplacement : public eda::gate::optimizer::CutVisitor {
  public:
    using RWDatabase = eda::gate::optimizer::RWDatabase;
    using BoundGNetList = eda::gate::optimizer::RWDatabase::BoundGNetList;
    using BoundGNet = eda::gate::optimizer::RWDatabase::BoundGNet;
    using CutStorage = eda::gate::optimizer::CutStorage;
    using VisitorFlags = eda::gate::optimizer::VisitorFlags;
    using CellTypeID = eda::gate::model::CellTypeID;

    SearchOptReplacement();

    void set(CutStorage *cutStorage, GNet *net,
        std::unordered_map<GateID, Replacement> *bestSubstitutions, 
        int cutSize, RWDatabase &rwdb, Strategy *strategy,
        std::unordered_map<std::string, CellTypeID> &cellTypeMap);

    VisitorFlags onNodeBegin(const GateID &) override;

    VisitorFlags onNodeEnd(const GateID &) override;

    VisitorFlags onCut(const GateID &, const Cut &) override;

    std::unordered_map<GateID, Replacement> *bestSubstitutions;

  private:
    GNet *net;
    CutStorage *cutStorage;
    RWDatabase rwdb;

    BoundGNet bestOption;
    std::unordered_map<GateID, GateID> bestOptionMap;
    std::unordered_map<std::string, CellTypeID> cellTypeMap;
    bool saveReplace;
    double minNodeArrivalTime;

    GateID lastNode;
    CutStorage::Cuts *lastCuts;
    int cutSize;
    std::vector<const CutStorage::Cut *> toRemove;

    Strategy *strategy;

    bool checkOptimize(const BoundGNet &superGate,
        const std::unordered_map<GateID, GateID> &map);

    VisitorFlags
    considerTechMap(BoundGNet &superGate,
        std::unordered_map<GateID, GateID> &map);

    void saveBestReplacement();

    virtual BoundGNetList getSubnets(uint64_t func);

    bool checkValidCut(const Cut &cut);

    double maxArrivalTime(const BoundGNet &superGate,
        const std::unordered_map<GateID, GateID> &map);
  };

} // namespace eda::gate::tech_optimizer
