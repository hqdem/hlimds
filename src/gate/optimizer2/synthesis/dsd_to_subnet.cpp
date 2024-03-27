//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "dsd_to_subnet.h"

namespace eda::gate::optimizer2::synthesis {

SubnetId DsdSynthesizer::synthesize(const BddWithDdManager &pair,
                                 uint16_t maxArity) {
  DSDNode *dsd;
  DSDManager *dmanager;
  /* Initialize DSD manager by choosing a starting cache size */
  dmanager = DSD_Init(pair.manager, Cudd_ReadMemoryInUse(pair.manager)/2);
  /* Create a DSD from a BDD */
  dsd = DSD_Create(dmanager, pair.bdd);
  /* Always reference after creation */
  DSD_Ref(dmanager, dsd);
  /* Debug print */
#ifdef UTOPIA_DEBUG
  Recursive_Decomposition_Print(dsd);
#endif

  SubnetBuilder subnetBuilder;
  LinkList inputsList;
  /* Create PIs */
  int numVars = Cudd_ReadSize(pair.manager);
  for (int i = 0; i < numVars; ++i) {
    inputsList.emplace_back(subnetBuilder.addInput());
  }

  /* To complete the subnet building connect the last gate - OUT */
  subnetBuilder.addCell(CellSymbol::OUT, buildNet(dsd,
                                                  dmanager,
                                                  subnetBuilder,
                                                  inputsList,
                                                  maxArity));
  SubnetId ret = subnetBuilder.make();
  DSD_Quit(dmanager);
  return ret;
}

SubnetId DsdSynthesizer::synthesize(const TruthTable &table, uint16_t maxArity) {
  /* Initial subnet */
  MMSynthesizer k;
  const auto &subnet = Subnet::get(k.synthesize(table, maxArity));

  /* Subnet to BDD convertion */
  Cudd manager(0, 0);
  // Output gate is the last element in entries array
  unsigned outputId = subnet.size() - 1;
  BDD netBDD = SubnetToBdd::convert(subnet, outputId, manager);

  /* Initialize DSD manager by choosing a starting cache size */
  DSDNode *dsd;
  DSDManager *dmanager;
  dmanager = DSD_Init(netBDD.manager(),
                      Cudd_ReadMemoryInUse(netBDD.manager())/2);
  /* Create a DSD from a BDD */
  dsd = DSD_Create(dmanager, netBDD.getNode());
  /* Always reference after creation */
  DSD_Ref(dmanager, dsd);

  SubnetBuilder subnetBuilder;
  LinkList inputsList;
  /* Create PIs */
  int numVars = Cudd_ReadSize(manager.getManager());
  for (int i = 0; i < numVars; ++i) {
    inputsList.emplace_back(subnetBuilder.addInput());
  }

  /* To complete the subnet building connect the last gate - OUT */
  subnetBuilder.addCell(CellSymbol::OUT, buildNet(dsd,
                                                  dmanager,
                                                  subnetBuilder,
                                                  inputsList,
                                                  maxArity));
  SubnetId ret = subnetBuilder.make();
  DSD_Quit(dmanager);
  return ret;
}

Link DsdSynthesizer::buildNet(DSDNode *dsd,
                           const DSDManager *dmanager,
                           SubnetBuilder &subnetBuilder,
                           const LinkList &inputsList,
                           uint16_t maxArity) {
  if (INPUT_SIZE(dsd) == 0) {
    // No actuals, so PI, either ZERO or ONE.
    if (Cudd_IsConstant(get_bdd(dsd))) {
      if (Cudd_IsComplement(get_bdd(dsd))) {
        return subnetBuilder.addCell(CellSymbol::ZERO);
      }
      return subnetBuilder.addCell(CellSymbol::ONE);
    }
    /* This is a PI and we need to find the number of this input in the
     * inputsList. Initially references to inputs are all positive. It
     * is necessary to add negation if there is one. */
    return Link(inputsList[Cudd_NodeReadIndex(get_bdd(dsd))].idx,
                DSD_IsComplement(dsd));
  }
  // We can add a gate when all its inputs are added to the link list.
  LinkList currentGateInputs;
  auto *iter = DSD_Regular(dsd)->actual_list;
  while (iter != NULL) {
    if (GET_TYPE(DSD_Regular(iter->decomposition)) == DSD_VAR) {
      if (Cudd_IsConstant(get_bdd(iter->decomposition))) {
        if (Cudd_IsComplement(get_bdd(iter->decomposition))) {
          return subnetBuilder.addCell(CellSymbol::ZERO);
        }
        return subnetBuilder.addCell(CellSymbol::ONE);
      }
      // Now we use the actual and not the DSD node itself.
      auto index = Cudd_NodeReadIndex(get_bdd(iter->decomposition));
      auto sign = DSD_IsComplement(iter->decomposition);
      currentGateInputs.emplace_back(Link(inputsList[index].idx, sign));
    } else {
      // If it's not a VAR, we go recursively deeper.
      Subnet::Link ret = buildNet(iter->decomposition,
                                  dmanager,
                                  subnetBuilder,
                                  inputsList,
                                  maxArity);
      currentGateInputs.emplace_back(ret);
    }
    iter = iter->next;
  }

  /*
   * At this point we got to a node that has every actual decomposed.
   * since we've eliminated that it might be a VAR type.
   * Then we work with the DSD node itself, not the actual.
  */
  switch (GET_TYPE(DSD_Regular(dsd))) {
    case DSD_PRIME:
      return decomposePrimeGate(dsd,
                                dmanager,
                                subnetBuilder,
                                currentGateInputs);
    break;
    case DSD_OR:
      return Link(subnetBuilder.addCellTree(CellSymbol::OR,
                                            currentGateInputs,
                                            maxArity).idx,
                  DSD_IsComplement(dsd));
    break;
    case DSD_XOR:
      return Link(subnetBuilder.addCellTree(CellSymbol::XOR,
                                            currentGateInputs,
                                            maxArity).idx,
                  DSD_IsComplement(dsd));
    break;
    default:
      assert(false && "Unknown gate type!");
      exit(EXIT_FAILURE);
    break;
  }
}

bool isDependentOnVariable(const DdManager* manager,
                           DdNode* bdd,
                           uint32_t variableIndex) {
  // If BDD is constant, then it does not depend on variables
  if (Cudd_IsConstant(bdd)) {
    return false;
  }

  // Checking the index of a variable in a node
  if (Cudd_NodeReadIndex(bdd) == variableIndex) {
    return true;
  }

  // Recursive verification of subtrees
  return isDependentOnVariable(manager, Cudd_T(bdd), variableIndex) ||
      isDependentOnVariable(manager, Cudd_E(bdd), variableIndex);
}

Link DsdSynthesizer::getLinkToCorrectActual(const DSDNode *dsd,
                                         const DdManager *manager,
                                         uint32_t variableIndex,
                                         const LinkList &inputsList) {
  size_t i = 0;
  auto *iter = DSD_Regular(dsd)->actual_list;
  while (iter != NULL) {
    auto *actual = get_bdd(iter->decomposition);
    /* Since in DSD significant variables in actuals cannot be repeated,
     * the first actual depending on the variable with the index is the only
     * correct one. */
    if (isDependentOnVariable(manager, actual, variableIndex)) {
      return inputsList[i];
    }
    iter = iter->next;
    ++i;
  }
  assert(false && "Actual is not found!");
  exit(EXIT_FAILURE);
}

Link DsdSynthesizer::decomposePrimeGate(DSDNode *dsd,
                                     const DSDManager *dmanager,
                                     SubnetBuilder &subnetBuilder,
                                     const LinkList &inputsList) {
  // If there is a negation in the DSD tree, enters it into BDD.
  auto *bdd = get_symbolic_kernel(dsd);
  Link ret = recursiveBddStep(bdd,
                              dmanager->Ddmanager_analogue,
                              subnetBuilder,
                              inputsList,
                              dsd);
  /* Each recursion step is responsible for the negation of BDD nodes below.
   * Since we have the last return here, we change the sign if necessary. */
  if (Cudd_IsComplement(bdd)) {
    return Subnet::Link(ret.idx, !ret.inv);
  }
  return ret;
}

Link DsdSynthesizer::recursiveBddStep(DdNode *bdd,
                                   const DdManager *manager,
                                   SubnetBuilder &subnetBuilder,
                                   const LinkList &inputsList,
                                   const DSDNode *dsd) {
  /*  T - "Then" path (node variable is true)
   *  E - "Else" path (node variable is false) */
  DdNode *t = Cudd_T(bdd);
  DdNode *e = Cudd_E(bdd);
  if ((!Cudd_IsConstant(e) and Cudd_IsConstant(t))
      or (Cudd_IsConstant(e) and !Cudd_IsConstant(t))) {
    /*
     * Constant and not constant case.
     * Four possible BDD constructions (with complementation *):
     *          c            c            c            c
     *         / \          / \          / \          / \
     *       1/   \0      1/   *0      1/   \0      1/   *0
     *       /     \      /     \      /     \      /     \
     *      1       d    1       d    d       1    d       1
     *         c+d         c+!d         !c+d          c*d
     */

    /*  NOTE: Prime gate inputs have no negation in DSD tree,
     *  but can have negation in BDD representation
     *  (e.g. one of the Prime gate inputs is also a Prime gate) */

    // Find the link corresponding to the top variable
    Link c = getLinkToCorrectActual(dsd,
                                    manager,
                                    Cudd_NodeReadIndex(bdd),
                                    inputsList);
    Link d;
    if (Cudd_IsConstant(e)) {
      d = recursiveBddStep(t, manager, subnetBuilder, inputsList, dsd);
      if (Cudd_IsComplement(e)) {
        // c*d
        Link result = subnetBuilder.addCell(CellSymbol::AND, c, d);
        return result;
      }
      // !c+d
      Link result = subnetBuilder.addCell(CellSymbol::OR,
                                          Link(c.idx, !c.inv),
                                          d);
      return result;
    }
    d = recursiveBddStep(e, manager, subnetBuilder, inputsList, dsd);
    if (Cudd_IsComplement(e)) {
      // c+!d
      Link result = subnetBuilder.addCell(CellSymbol::OR,
                                          c,
                                          Link(d.idx, !d.inv));
      return result;
    }
    // c+d
    Link result = subnetBuilder.addCell(CellSymbol::OR, c, d);
    return result;
  }

  if (Cudd_IsConstant(e) and Cudd_IsConstant(t)) {
    /* Since we are using a symbolic kernel, this node is Prime gate input.
     * This is the lowest level, signs are not considered.
     * PS: This cannot be the top node, because Prime has >= 3 inputs. */

    return getLinkToCorrectActual(dsd, manager,
                                  Cudd_NodeReadIndex(bdd),
                                  inputsList);
  }

  /* Each BDD node with topvar = z and T = a, E = b is function z*a + !z*b
   * Make one step deeper by recursive */

  // The dsd node is passed to get the list of actuals
  Link a = recursiveBddStep(t, manager, subnetBuilder, inputsList, dsd);
  Link b = recursiveBddStep(e, manager, subnetBuilder, inputsList, dsd);

  // Possible negation on the "Else" path, change a sign to the opposite one.
  if (Cudd_IsComplement(e)) {
    b = Subnet::Link(b.idx, !b.inv);
  }

  // Create valves according to the formula z*a + !z*b
  Link z = getLinkToCorrectActual(dsd,
                                  manager,
                                  Cudd_NodeReadIndex(bdd),
                                  inputsList);
  Link za = subnetBuilder.addCell(CellSymbol::AND, z, a);
  Link notzb = subnetBuilder.addCell(CellSymbol::AND,
                                     Link(z.idx, !z.inv),
                                     b);
  Link result = subnetBuilder.addCell(CellSymbol::OR, za, notzb);
  return result;
}

}; // namespace eda::gate::optimizer2::synthesis
