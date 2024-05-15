//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/array.h"
#include "gate/mutator/mutator.h"

namespace eda::gate::mutator {

  using CellID = eda::gate::model::CellID;
  using CellIDList = std::vector<CellID>;
  using CellSymbol = eda::gate::model::CellSymbol;
  using CellSymbolList = std::vector<CellSymbol>;
  using Entry = eda::gate::model::Subnet::Entry;
  using EntryArray = eda::gate::model::Array<Entry>;
  using Link = eda::gate::model::Subnet::Link;
  using LinkList = eda::gate::model::Subnet::LinkList;
  using Subnet = eda::gate::model::Subnet;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;

  /**
   * \brief Class for mutating Subnets
   * \author <a>Rzhevskaya Maria</a>
   * \details This class mutates cells in Nets depending on the 
   * input values
  */
  class MutatorTransformer {
    CellIDList replacedCells;
    CellSymbolList replacedFunc;
    unsigned int numCells = 0;
    unsigned int numMutated = 0;
    std::unordered_map<CellID, CellIDList> childCellList;
  public:
    //===------------------------------------------------------------------===//
    // Constructor
    //===------------------------------------------------------------------===//
    /**
     * \brief Initializes transformer for cell-level mutant generation.
     * \param[in] inputNet input net that will be mutated
     * \param[in] subnetBuilder builder is used for creating mutated net
     * \param[in] numOfCells The number of cells that will be mutated
     * \param[in] cellIdList The list of cells that need to 
     * be mutated if requirements are met
     * \param[in] function The list of cell's symbol based on which 
     * the cell from the previous list will be mutated
     * \details This constuctor also transform cells.
     * The list of functions shows exactly which cells 
     * will be mutated, that is if cell's symbol is not in the list, 
     * this cell will not be mutated.
    */
    MutatorTransformer(Subnet &inputNet,
                        SubnetBuilder &subnetBuilder,
                        int numOfCells,
                        CellIDList &cellIdList,
                        CellSymbolList &function);
    ~MutatorTransformer() = default;

    /// Function returns number of mutated cells
    unsigned int getNumMutatedCells() {
      return numMutated;
    }
    /// Function returns list of mutated cells
    CellIDList getMutatedCellsList() {
      return replacedCells;
    }
  private:
    bool connectedWithOut(const EntryArray entries,
                          const CellID &startCell);
    CellIDList filterListCell(const EntryArray &entries);
    void addMutatedCell(SubnetBuilder &subnetBuilder,
                        const EntryArray &entries,
                        const CellID &cellID,
                        const LinkList &linkList);
    void findChildren(Subnet &inputNet,
                      const EntryArray &entries);
  };
  
} // namespace eda::gate::mutator
