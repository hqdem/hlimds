//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#ifndef SUBNET_TO_BDD_CONVERTER_H
#define SUBNET_TO_BDD_CONVERTER_H

#include "gate/model2/subnet.h"

#include "cuddObj.hh"

#include <map>

namespace eda::gate::model::utils {

//===--------------------------------------------------------------------===//
// Types
//===--------------------------------------------------------------------===//

using SignedBDDList = std::vector<std::pair<BDD, bool>>;
using CellBDDMap = std::map<unsigned, BDD>;
using BDDList = std::vector<BDD>;

/**
* \brief Converts Subnet to BDD (Binary Decision Diagram) form.
*/
class SubnetToBdd {
public:
  /**
  * Converts only one gate of the net.
  * @param net Link to current Subnet.
  * @param cellId Entry index of the gate.
  * @param manager Link to Cudd manager.
  * @return resulting BDD.
  */
  static BDD convert(const Subnet &net,
                     unsigned cellId,
                     const Cudd &manager);

  /**
  * Converts list of gates of the net.
  * @param net Link to current Subnet.
  * @param list List of gate indexes.
  * @param manager Link to Cudd manager.
  * @return vector of resulting BDDs.
  */
  static BDDList convertList(const Subnet &net,
                             std::vector<unsigned> &list,
                             const Cudd &manager);

private:
  /**
  * Converts the whole net.
  * @param net Link to current Subnet.
  * @param outputSignedBDDList Link to list of BDDs, corresponding to Entries.
  * @param manager Link to Cudd manager.
  */
  static void convertAll(const Subnet &net,
                         SignedBDDList &outputSignedBDDList,
                         const Cudd &manager);

  /**
  * Represents BDD analogue for a Gate.
  * @param func Type of gate.
  * @param inputList Pointer to the DSD scheme.
  * @param manager Link to list of inputs represented in BDD.
  * @return BDD for current gate.
  */
  static BDD applyGateFunc(const CellSymbol func,
                           const SignedBDDList &inputList,
                           const Cudd &manager);
};

}; // namespace eda::gate::model::utils

#endif // SUBNET_TO_BDD_CONVERTER_H
