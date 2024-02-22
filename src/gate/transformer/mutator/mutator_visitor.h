//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/optimizer/visitor.h"

#include <list>

namespace eda::gate::mutator {

  using GateId = eda::gate::model::GNet::GateId;
  using GateIdList = eda::gate::model::GNet::GateIdList;
  using GateSymbol = eda::gate::model::GateSymbol;
  using GateSymbolList = std::vector<GateSymbol>;
  using VisitorFlags = eda::gate::optimizer::VisitorFlags;
  using Visitor = eda::gate::optimizer::Visitor;

  /**
   * \brief Class for mutating GNets
   * \author <a>Rzhevskaya Maria</a>
   * \details This class mutates gates in Nets depending on the 
   * input values
  */
  class MutatorVisitor : public Visitor {
    GateIdList replacedGates;
    GateSymbolList replacedFunc;
    GNet mVGNet;
    unsigned int numChangedGates = 0;
    unsigned int numGates;
    std::unordered_map<GateId, GateIdList> childGateList;
  public:

    //===------------------------------------------------------------------===//
    // Constructor
    //===------------------------------------------------------------------===//
    /**
     * \brief Initializes visitor for gate-level mutant generation.
     * \param[in] inputGNet input net that will be mutated
     * \param[in] numOfGates The number of gates that will be mutated
     * \param[in] listGates The list of gates that need to 
     * be mutated if requirements are met
     * \param[in] listSymbol The list of gate's symbol based on which 
     * the gate from the 
     * previous list will be mutated
     * \details The list of functions shows exactly which gates 
     * will be mutated, that is if gate's symbol is not in the list, 
     * this gate will not be mutated.
    */
    MutatorVisitor(const GNet &inputGNet, 
                  int numOfGates,
                  GateIdList &listGates,
                  GateSymbolList &listSymbol);
    /// Function returns mutated net
    GNet getGNet() {
      return mVGNet;
    }
    /// Function returns number of mutated gates
    int getNumChangedGates() {
      return numChangedGates;
    }

    GateIdList listMutatedGate() {
      return replacedGates;
    }
    
private:
    /// Function from class Visitor              
    VisitorFlags onNodeBegin(const GateId &) override;
    /// Function from class Visitor    
    VisitorFlags onNodeEnd(const GateId &) override;

    /**
     * Function mutates Gates in net
     * \param GateId The id of the gate which will be mutated if 
     * requirements are met
    */
    void changeGate(const GateId &gateId);

    /**
     * Function finds path to out from gate
     * \param startGate The id of the gate to find the path
     * to the output gate for
     * \return result bool variable that stores true if the path
     * was found, otherwise false
    */
    bool connectedWithOut(const GateId &startGate);

    GateIdList filterListGate(GateIdList &listGate);
  };
} //namespace eda::gate::mutator

