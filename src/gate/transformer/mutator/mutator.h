//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/walker.h"
#include "gate/transformer/mutator/mutator_visitor.h"

namespace eda::gate::mutator {

  using GateId = eda::gate::model::GNet::GateId;
  using GateIdList = eda::gate::model::GNet::GateIdList;
  using GateSymbol = eda::gate::model::GateSymbol;
  using GateSymbolList = std::vector<GateSymbol>;
  using GNet = eda::gate::model::GNet;
  using OperatingMode = bool;
  
  /// function for static functions of Mutator. It makes the list of gates which will be replaced in clone GNet
  GateIdList makeListReplacedGates(const GNet &inputGNet, 
                                   GateIdList &listGates);

  /// function for static functions of Mutator. It runs visitor and walker for mutator
  MutatorVisitor runVisitor(GNet &inputGNet, 
                            int numberOfGates,
                            GateIdList &listGates,
                            GateSymbolList function);

  /**
   * \brief Class for mutating nets
   * \author <a>Rzhevskaya Maria</a>
   * \details This class runs functions to create the necessary objects
   * for subsequent net mutation and creates a class object
   * that mutates the logical net
  */
  class Mutator {
  private:
    //===--------------------------------------------------------------------===//
    // Constructor and destructor
    //===--------------------------------------------------------------------===//
    Mutator() = default;
    ~Mutator() = default;

  public:
    //===--------------------------------------------------------------------===//
    // Static functions
    //===--------------------------------------------------------------------===//
    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] inputGNet input net that will be mutated
     * \param[in] listGates The list of gates that need to be mutated if requirements are met
     * \param[in] function The list of gate's symbol based on which the gate from the 
     * previous list will be mutated
     * \return mutated net
     * \details The list of functions shows exactly which gates will be mutated, that is if gate's symbol is not in the list, 
     * this gate will not be mutated
    */
    static GNet mutate(GNet &inputGNet, 
                       GateIdList &listGates,
                       GateSymbolList function = {GateSymbol::AND});

    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] counter is equivalent the number of mutated gates
     * \param[in] inputGNet input net that will be mutated
     * \param[in] numberOfGates The number of gates that will be mutated
     * \param[in] function The list of gate's symbol based on which the certain number of gates will be mutated
     * \return mutated net
     * \details The list of functions shows exactly which gates will be mutated, that is if gate's symbol is not in the list, 
     * this gate will not be mutated
    */
    static GNet mutate(int &counter,
                       GNet &inputGNet,
                       int numberOfGates,
                       GateSymbolList function = {GateSymbol::AND});
    
    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] inputGNet input net that will be mutated
     * \param[in] numberOfGates The number of gates that will be mutated
     * \param[in] function The list of gate's symbol based on which the certain number of gates will be mutated
     * \return mutated net
     * \details The list of functions shows exactly which gates will be mutated, that is if gate's symbol is not in the list, 
     * this gate will not be mutated
    */
    static GNet mutate(GNet &inputGNet,
                       int numberOfGates,
                       GateSymbolList function = {GateSymbol::AND});
  };
} // namespace eda::gate::mutator
