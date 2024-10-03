//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "subnetview_to_bdd.h"

namespace eda::gate::model::utils {

DdNode *applyBinaryOperation(
    DdManager *manager,
    const Subnet::LinkList &inputList,
    SubnetBuilder &builder,
    std::function<DdNode*(DdManager*, DdNode*, DdNode*)> binaryOp,
    bool invertResult = false) {

  DdNode *result = const_cast<DdNode*>(builder.getDataPtr<DdNode>(
      inputList[0].idx));

  result = inputList[0].inv ? Cudd_Not(result) : result;

  DdNode *secondOperand = nullptr;
  for (size_t i = 1; i < inputList.size(); i++) {
    DdNode *ith = const_cast<DdNode*>(
        builder.getDataPtr<DdNode>(inputList[i].idx));
    secondOperand = inputList[i].inv ? Cudd_Not(ith) : ith;
    result = binaryOp(manager, result, secondOperand);
    Cudd_Ref(result);
  }

  return invertResult ? Cudd_Not(result) : result;
}

DdNode *applyGateFunc(
    const CellSymbol func,
    const Subnet::LinkList &inputList,
    const Cudd &cudd,
    SubnetBuilder &builder) {

  DdManager *manager = cudd.getManager();
  DdNode *result = nullptr;
  DdNode *first = nullptr;

  if (inputList.size() >= 1) {
    first = const_cast<DdNode*>(builder.getDataPtr<DdNode>(inputList[0].idx));
  }

  switch (func) {
  case CellSymbol::ZERO:
    result = Cudd_ReadLogicZero(manager);
    Cudd_Ref(result);
    break;
  case CellSymbol::ONE:
    result = Cudd_ReadOne(manager);
    Cudd_Ref(result);
    break;
  case CellSymbol::BUF:
  case CellSymbol::IN:
  case CellSymbol::OUT:
    assert(inputList.size() == 1);
    result = inputList[0].inv ? Cudd_Not(first) : first;
    break;
  case CellSymbol::NOT:
    assert(inputList.size() == 1);
    result = Cudd_Not(first);
    break;
  case CellSymbol::AND:
    result = applyBinaryOperation(manager, inputList, builder, Cudd_bddAnd);
    break;
  case CellSymbol::OR:
    result = applyBinaryOperation(manager, inputList, builder, Cudd_bddOr);
    break;
  case CellSymbol::XOR:
    result = applyBinaryOperation(manager, inputList, builder, Cudd_bddXor);
    break;
  case CellSymbol::NAND:
    result = applyBinaryOperation(manager, inputList, builder, Cudd_bddAnd,
        true);
    break;
  case CellSymbol::NOR:
    result = applyBinaryOperation(manager, inputList, builder, Cudd_bddOr,
        true);
    break;
  case CellSymbol::XNOR:
    result = applyBinaryOperation(manager, inputList, builder, Cudd_bddXor,
        true);
    break;
  // Only supports 3 arguments
  case CellSymbol::MAJ: {
    DdNode *second = const_cast<DdNode*>(
        builder.getDataPtr<DdNode>(inputList[1].idx));
    DdNode *third = const_cast<DdNode*>(
        builder.getDataPtr<DdNode>(inputList[2].idx));
    assert(inputList.size() == 3 &&
           "BDD converter only supports majority function of 3 arguments.");
    DdNode *x1 = inputList[0].inv ? Cudd_Not(first) : first;
    DdNode *x2 = inputList[1].inv ? Cudd_Not(second) : second;
    DdNode *x3 = inputList[2].inv ? Cudd_Not(third) : third;
    DdNode *x1x2 = Cudd_bddAnd(manager, x1, x2);
    Cudd_Ref(x1x2);
    DdNode *x1x3 = Cudd_bddAnd(manager, x1, x3);
    Cudd_Ref(x1x3);
    DdNode *x2x3 = Cudd_bddAnd(manager, x2, x3);
    Cudd_Ref(x2x3);
    DdNode *temp = Cudd_bddOr(manager, x1x2, x1x3);
    Cudd_Ref(temp);
    result = Cudd_bddOr(manager, temp, x2x3);
    Cudd_Ref(result);
    break;
  }
  default:
    assert(false && "Unsupported gate");
    break;
  }
  return result;
}

BddList convertBdd(const SubnetView &sv, const Cudd &cudd) {
  BddList result;
  SubnetViewWalker walker(sv);

  // There's no guarantee walker will reach PO in descending ID order.
  std::unordered_map<EntryID, BDD> map;
  walker.run([&map, &cudd](
      SubnetBuilder &builder,
      const bool isIn,
      const bool isOut,
      const EntryID id) -> bool {
    DdManager *manager = cudd.getManager();
    const auto &cell = builder.getEntry(id).cell;
    DdNode *bddNode = nullptr;

    if (isIn) {
      bddNode = Cudd_bddIthVar(manager, id);
      Cudd_Ref(bddNode);
    } else {
      bddNode = applyGateFunc(
        cell.getSymbol(),
        builder.getLinks(id), cudd, builder);
    }

    builder.setDataPtr(id, bddNode);

    if (isOut) {
      assert(bddNode != nullptr && "Empty pointer to current BDD node!");
      map[id]= {cudd, bddNode};
    }
    return true;
  });

  // Sort BDD's by PO EntryID
  for (auto i: sv.getOutputs()) {
    result.push_back(map[i]);
  }

  return result;
}

} // namespace eda::gate::model::utils
