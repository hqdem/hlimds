//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#ifndef DSD_TO_GNET_H
#define DSD_TO_GNET_H

#include "gate/function/bdd.h"
#include "gate/model/subnet.h"
#include "gate/model/subnetview.h"
#include "gate/model/utils/subnetview_to_bdd.h"
#include "gate/optimizer/synthesis/isop.h"
#include "gate/optimizer/synthesizer.h"
#include "util/logging.h"

#include "kitty/kitty.hpp"
extern "C" {
  #include "DSDInterface.h"
}

static constexpr int DSD_VAR = VAR;
static constexpr int DSD_PRIME = PRIME;
static constexpr int DSD_OR = OR;
static constexpr int DSD_XOR = XOR;

#undef VAR
#undef PRIME
#undef OR
#undef XOR

namespace eda::gate::optimizer::synthesis {

//===--------------------------------------------------------------------===//
// Types
//===--------------------------------------------------------------------===//

using CellSymbol = eda::gate::model::CellSymbol;
using Subnet = eda::gate::model::Subnet;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetID = eda::gate::model::SubnetID;
using SubnetObject = eda::gate::model::SubnetObject;
using SubnetView = eda::gate::model::SubnetView;
using Link = Subnet::Link;
using LinkList = Subnet::LinkList;

/**
 * \brief BDD node and DdManager pair for passing to synthesize.
*/
struct BddWithDdManager {
  DdNode *bdd;
  DdManager *manager;
};

/**
 * \brief Implements functions for transferring DSD to a Subnet.
 *
 * The Disjoint Support Decomposition (DSD) algorithm based on
 * the article "Achieving Scalable Hardware Verification
 * with Symbolic Simulation" by Valeria Bertacco (2003).
*/
class DsdSynthesizer : public BddSynthesizer,
                       public TruthTableSynthesizer {

public:

  using TruthTable = util::TruthTable;

  //===--------------------------------------------------------------------===//
  // Constructors/Destructors
  //===--------------------------------------------------------------------===//

  /// Empty constructor.
  DsdSynthesizer() {}

  //===--------------------------------------------------------------------===//
  // Convenience Methods
  //===--------------------------------------------------------------------===//

  using BddSynthesizer::synthesize;
  using TruthTableSynthesizer::synthesize;

  /// Synthesize.
  SubnetObject synthesize(const model::Bdd &bdd, const TruthTable &,
                          uint16_t maxArity = -1) const override;
  SubnetObject synthesize(const TruthTable &table, const TruthTable &care,
                          uint16_t maxArity = -1) const override;

private:

  //===--------------------------------------------------------------------===//
  // Internal Methods
  //===--------------------------------------------------------------------===//

  /**
  * It builds the subnet without connecting the last OUT gate.
  * @param dsd Pointer to the DSD node.
  * @param dmanager Pointer to the DSD manager
  * @param subnetBuilder Link to a SubnetBuilder.
  * @param inputsList List of IN gates.
  * @param maxArity Max arity of gates.
  * @return Link to the gate to which OUT should to be connected.
  */
  Link buildNet(DSDNode *dsd,
                const DSDManager *dmanager,
                SubnetBuilder &subnetBuilder,
                const LinkList &inputsList,
                uint16_t maxArity = -1) const;

  /**
  * Starts the PRIME gate decomposition procedure via bdd representation.
  * @param dsd Pointer to the DSD node.
  * @param dmanager Pointer to the DSD manager.
  * @param subnetBuilder Link to a SubnetBuilder.
  * @param inputsList List of inputs of this Prime gate.
  * @return Link to the gate corresponding to the output of the PRIME gate.
  */
  Link decomposePrimeGate(DSDNode *dsd,
                          const DSDManager *dmanager,
                          SubnetBuilder &subnetBuilder,
                          const LinkList &inputsList) const;

  /**
  * Select the actual whose BDD depends on a variable.
  * @param dsd Pointer to the DSD node (used to get actuals list).
  * @param manager Pointer to the CUDD manager.
  * @param variableIndex CUDD variable index whose dependency is being tested.
  * @param inputsList List of inputs of this Prime gate.
  * @return Link to an input whose BDD depends on a variable with an index.
  */
  Link getLinkToCorrectActual(const DSDNode *dsd,
                              const DdManager *manager,
                              uint32_t variableIndex,
                              const LinkList &inputsList) const;

  /**
  * Recursive step of PRIME gate depomposition.
  * @param bdd Pointer to the BDD node (symbolic kernel).
  * @param manager Pointer to the CUDD manager.
  * @param subnetBuilder Link to a SubnetBuilder.
  * @param inputsList List of inputs of this Prime gate.
  * @param dsd Pointer to the DSD node.
  * @return Link to an input whose BDD depends on a variable with an index.
  */
  Link recursiveBddStep(DdNode *bdd,
                        const DdManager *manager,
                        SubnetBuilder &subnetBuilder,
                        const LinkList &inputsList,
                        const DSDNode *dsd) const;
};

/**
* Checks whether the actual is dependent on variable with such an index.
* @param manager Pointer to the CUDD manager.
* @param bdd Pointer to the BDD node (symbolic kernel).
* @param variableIndex CUDD variable index whose dependency is being tested.
* @return One if it depends, otherwise zero.
*/
bool isDependentOnVariable(const DdManager *manager,
                           DdNode *bdd,
                           uint32_t variableIndex);

}; //namespace eda::gate::optimizer::synthesis
#endif // DSD_TO_GNET_H
