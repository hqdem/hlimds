
#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/rwdatabase.h"

#include <unordered_map>

namespace eda::gate::optimizer {
/**
 * \brief Struct for descriptions Supper Gate
 * \author <a href="mailto:dgaryaev@ispras.ru"></a>
 */
  struct Replacement {
    using GNet = model::GNet;
    using GateID = GNet::GateId;
    using BoundGNet = RWDatabase::BoundGNet;

    GateID rootNode;
    std::unordered_map<GateID, GateID> bestOptionMap;
    GNet *subsNet;
    GNet *net;
    double delay;
    std::string name;
    double area;
    //std::unordered_map<uint32_t, double> delays; 
  };
} // namespace eda::gate::optimizer
