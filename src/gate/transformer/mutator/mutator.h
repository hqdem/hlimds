//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"
#include "gate/optimizer/optimizer.h"
#include "gate/optimizer/walker.h"
#include "gate/transformer/mutator/mutator_visitor.h"

namespace eda::gate::mutator {

  using Cut = eda::gate::optimizer::CutStorage::Cut;
  using Cuts = eda::gate::optimizer::CutStorage::Cuts;
  using CutStorage = eda::gate::optimizer::CutStorage;
  using GateId = eda::gate::model::GNet::GateId;
  using GateIdList = eda::gate::model::GNet::GateIdList;
  using GateSymbol = eda::gate::model::GateSymbol;
  using GateSymbolList = std::vector<GateSymbol>;
  using GNet = eda::gate::model::GNet;
  using eda::gate::optimizer::findCuts;
  
  /**
   * \brief mutator modes
   * \details CUT mode indicates that mutator modifies the current 
   * number of subnets or the subnets. Subnets includes gate, cut
   * of maximum size for this gate and gates between the gate and
   * its cut of current gates.
   * GATE mode indicates that mutator modifies the current 
   * number of gates or current gates
  */
  enum MutatorMode {
    GATE = 0,
    CUT = 1
  };

  /// Makes list of gates and their cuts transformed into list of gates
  GateIdList makeListGate(GNet &inputGNet,
                          unsigned int numOfCuts,
                          GateIdList &listGates,
                          unsigned int cutSize);
  
  /// Makes list of gates are to be replaced in mutant net.
  GateIdList makeListReplacedGates(const GNet &inputGNet, 
                                   GateIdList &listGates);


  /// Creates parameters for mutator visitor
  int paramForVisitor(MutatorMode mode,
                      GNet &inputGNet, 
                      unsigned int &number,
                      GateIdList &gatesList,
                      unsigned int &cutSize);
                      
  /// Runs visitor and walker for mutator.
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
    //===------------------------------------------------------------------===//
    // Constructor and destructor
    //===------------------------------------------------------------------===//
    Mutator() = default;
    ~Mutator() = default;

  public:
    //===------------------------------------------------------------------===//
    // Static functions
    //===------------------------------------------------------------------===//
    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] mode defines the mode of operation of the mutator
     * \param[in] inputGNet input net that will be mutated
     * \param[in] listGates the list of gates which or for which cuts 
     * need to be mutated if requirements are met
     * \param[in] function the list of gate's symbol based on which 
     * the gate from the 
     * previous list will be mutated
     * \param[in] cutSize maximum size of cuts
     * \return mutated net
     * \details The list of functions shows exactly which gates will 
     * be mutated, that is if gate's symbol is not in the list, 
     * this gate will not be mutated. 
    */
    static GNet mutate(MutatorMode mode,
                       GNet &inputGNet, 
                       GateIdList &listGates,
                       GateSymbolList function = {GateSymbol::AND},
                       unsigned int cutSize = 1);

    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] mode defines the mode of operation of the mutator
     * \param[in] counter equivalent the number of mutated gates
     * \param[in] inputGNet input net that will be mutated
     * \param[in] num depending on mutator mode, it can be the number 
     * of gates or cuts that will be mutated
     * \param[in] function the list of gate's symbol based on which 
     * the certain number of gates will be mutated
     * \param[in] cutSize maximum size of cuts
     * \return mutated net
     * \details The list of functions shows exactly which gates will 
     * be mutated, that is if gate's symbol is not in the list, 
     * this gate will not be mutated. 
    */
    static GNet mutate(MutatorMode mode,
                       int &counter,
                       GNet &inputGNet,
                       unsigned int num,
                       GateSymbolList function = {GateSymbol::AND},
                       unsigned int cutSize = 1);
    
    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] mode defines the mode of operation of the mutator
     * \param[in] mutatedGates is list with mutated gates
     * \param[in] inputGNet input net that will be mutated
     * \param[in] num depending on mutator mode, it can be the number 
     * of gates or cuts that will be mutated
     * \param[in] function the list of gate's symbol based on which 
     * the certain number of gates will be mutated
     * \param[in] cutSize maximum size of cuts
     * \return mutated net
     * \details The list of functions shows exactly which gates will 
     * be mutated, that is if gate's symbol is not in the list, 
     * this gate will not be mutated. 
    */
    static GNet mutate(MutatorMode mode,
                       GateIdList &mutatedGates,
                       GNet &inputGNet,
                       unsigned int num,
                       GateSymbolList function = {GateSymbol::AND},
                       unsigned int cutSize = 1);
    
    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] mode defines the mode of operation of the mutator
     * \param[in] inputGNet input net that will be mutated
     * \param[in] num depending on mutator mode, it can be the number 
     * of gates or cuts that will be mutated
     * \param[in] function the list of gate's symbol based on which 
     * the certain number of gates will be mutated
     * \param[in] cutSize maximum size of cuts
     * \return mutated net
     * \details The list of functions shows exactly which gates 
     * will be mutated, that is if gate's symbol is not in the list, 
     * this gate will not be mutated. 
    */
    static GNet mutate(MutatorMode mode,
                       GNet &inputGNet,
                       unsigned int num,
                       GateSymbolList function = {GateSymbol::AND},
                       unsigned int cutSize = 1);
  };
} // namespace eda::gate::mutator

