
#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/net_substitute.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/model2/celltype.h"

#include <unordered_map>

namespace eda::gate::tech_optimizer {
/**
 * \brief Struct for descriptions Super Gate
 * \author <a href="mailto:dgaryaev@ispras.ru"></a>
 */
  struct Replacement {
    using GNet = model::GNet;
    using GateID = GNet::GateId;
    using BoundGNet = eda::gate::optimizer::RWDatabase::BoundGNet;
    using NetSubstitute = eda::gate::optimizer::NetSubstitute;
    using CellType = eda::gate::model::CellTypeID;

    Replacement(GateID rootNode,
                NetSubstitute netSubstitute,
                double delay,
                std::string name,
                double area,
                model::CellTypeID cellType) : rootNode(rootNode),
                            netSubstitute(netSubstitute),
                            delay(delay), name(name),
                            area(area), cellType(cellType) {
    }

    Replacement(GateID rootNode,
                NetSubstitute netSubstitute,
                double delay,
                std::string name,
                double area, 
                optimizer::ConeVisitor::MatchMap *map) : rootNode(rootNode),
                            netSubstitute(netSubstitute),
                            delay(delay), name(name),
                            area(area), map(map) {
    }

    GateID rootNode;
    NetSubstitute netSubstitute;
    double delay;
    std::string name;
    double area;
    model::CellTypeID cellType;
    bool used = false;

    optimizer::ConeVisitor::MatchMap *map;

  };
} // namespace eda::gate::tech_optimizer
