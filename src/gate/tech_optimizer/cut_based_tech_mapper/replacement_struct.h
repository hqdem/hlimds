
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
    using CellType = eda::gate::model::CellType;

    GateID rootNode;
    NetSubstitute netSubstitute;
    double delay;
    std::string name;
    double area;
    CellType cellType;
  };
} // namespace eda::gate::tech_optimizer
