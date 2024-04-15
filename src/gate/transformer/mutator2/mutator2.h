//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/optimizer2/cut_extractor.h"
#include "gate/transformer/mutator2/mutator2_transformer.h"

#include <iostream>
#include <list>
#include <vector>

namespace eda::gate::mutator {

  using Cell = eda::gate::model::Subnet::Cell;
  using CellID = eda::gate::model::CellID;
  using CellIDList = std::vector<CellID>;
  using CellSymbol = eda::gate::model::CellSymbol;
  using CellSymbolList = std::vector<CellSymbol>;
  using Cut = eda::gate::optimizer2::CutExtractor::Cut;
  using CutExtractor = eda::gate::optimizer2::CutExtractor;
  using Link = eda::gate::model::Subnet::Link;
  using LinkList = eda::gate::model::Subnet::LinkList;
  using Subnet = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using SubnetID = eda::gate::model::SubnetID;

  /**
   * \brief mutator modes
   * \details CUT mode indicates that mutator modifies the current 
   * number of subnets. Subnet includes cell, cut
   * of maximum size for this cell and cells between the cell and
   * its cut of current cells.
   * CELL mode indicates that mutator modifies the current 
   * number of cells or current cells
  */
  enum Mutator2Mode {
    CELL = 0,
    CUT = 1
  };

  /**
   * \brief Class for mutating nets
   * \author <a>Rzhevskaya Maria</a>
   * \details This class runs functions to create the necessary objects
   * for subsequent net mutation and creates a class object
   * that mutates the logical net
  */
  class Mutator2 {
  private:
    //===------------------------------------------------------------------===//
    // Constructor and destructor
    //===------------------------------------------------------------------===//
    Mutator2() = default;
    ~Mutator2() = default;

  public:
    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] mode defines the mode of operation of the mutator
     * \param[in] inputNet input net that will be mutated
     * \param[in] cellIdList the list of cells' ID which need to be
     * mutated if requirements are met
     * \param[in] function the list of cell's symbol based on which 
     * the cell from the previous list will be mutated
     * \param[in] cutSize maximum size of cuts
     * \return SubnetBuilder with mutated subnet
     * \details The list of functions shows exactly which cells will 
     * be mutated, that is if cell's symbol is not in the list, 
     * this cell will not be mutated. 
    */
    static SubnetID mutate(Mutator2Mode mode,
                           Subnet &inputNet,
                           CellIDList &cellIdList,
                           CellSymbolList function = {CellSymbol::AND},
                           unsigned int cutSize = 1);

    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] mode defines the mode of operation of the mutator
     * \param[in] inputNet input net that will be mutated
     * \param[in] num depending on mutator mode, it can be the number 
     * of cells or cuts that will be mutated
     * \param[in] function the list of cell's symbol based on which 
     * the cell from the previous list will be mutated
     * \param[in] cutSize maximum size of cuts
     * \return SubnetBuilder with mutated subnet
     * \details The list of functions and number of cells shows how much 
     * cells will be mutated, that is if cell's symbol is not in the list, 
     * this cell will not be mutated.
    */
    static SubnetID mutate(Mutator2Mode mode,
                           Subnet &inputNet,
                           unsigned int num,
                           CellSymbolList function = {CellSymbol::AND},
                           unsigned int cutSize = 1); 
    
    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] mode defines the mode of operation of the mutator
     * \param[in] counter number of mutated cells
     * \param[in] inputNet input net that will be mutated
     * \param[in] num depending on mutator mode, it can be the number 
     * of cells or cuts that will be mutated
     * \param[in] function the list of cell's symbol based on which 
     * the cell from the previous list will be mutated
     * \param[in] cutSize maximum size of cuts
     * \return SubnetBuilder with mutated subnet
     * \details The list of functions and number of cells shows how much 
     * cells will be mutated, that is if cell's symbol is not in the list, 
     * this cell will not be mutated.
    */
    static SubnetID mutate(Mutator2Mode mode,
                           int &counter,
                           Subnet &inputNet,
                           unsigned int num,
                           CellSymbolList function = {CellSymbol::AND},
                           unsigned int cutSize = 1);  

    /**
     * \brief Creates a mutant net from the specified one.
     * \param[in] mode defines the mode of operation of the mutator
     * \param[in] mutatedCells is list with mutated cells
     * \param[in] inputNet input net that will be mutated
     * \param[in] num depending on mutator mode, it can be the number 
     * of cells or cuts that will be mutated
     * \param[in] function the list of cell's symbol based on which 
     * the cell from the previous list will be mutated
     * \param[in] cutSize maximum size of cuts
     * \return SubnetBuilder with mutated subnet
     * \details The list of functions and number of cells shows how much 
     * cells will be mutated, that is if cell's symbol is not in the list, 
     * this cell will not be mutated.
    */
    static SubnetID mutate(Mutator2Mode mode,
                           CellIDList &mutatedCells,
                           Subnet &inputNet,
                           unsigned int num,
                           CellSymbolList function = {CellSymbol::AND},
                           unsigned int cutSize = 1);                            
  };
} // namespace eda::gate::mutator
