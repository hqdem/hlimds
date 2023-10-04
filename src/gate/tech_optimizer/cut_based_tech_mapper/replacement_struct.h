
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
                model::CellTypeID cellType,
                std::string name,
                double delay,
                double area) : rootNode(rootNode),
                            cellType(cellType),
                            name(name), delay(delay),
                            area(area), isInput(true) {}

    Replacement(GateID rootNode,
                model::CellTypeID cellType,
                optimizer::ConeVisitor::MatchMap map,
                std::string name,
                double delay,
                double area) : rootNode(rootNode),
                            cellType(cellType), map(map),
                            name(name), delay(delay),
                            area(area) {}

    GateID rootNode;
    model::CellTypeID cellType;
    optimizer::ConeVisitor::MatchMap map;
    bool isInput = false;
    bool used = false;
    eda::gate::model::CellID cellID;

    //NetSubstitute netSubstitute;
    std::string name;
    double delay;
    double area;
  };
} // namespace eda::gate::tech_optimizer
