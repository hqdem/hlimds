//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "subnet_to_bdd.h"

namespace eda::gate::model::utils {

BDD SubnetToBdd::applyGateFunc(const CellSymbol func,
                               const SignedBDDList &inputList,
                               const Cudd &manager) {
  BDD result;
  BDD secondOperand;
  switch (func) {
  case CellSymbol::ZERO:
    result = manager.bddZero();
    break;
  case CellSymbol::ONE:
    result = manager.bddOne();
    break;
  case CellSymbol::BUF:
    assert(inputList.size() == 1);
    result = inputList[0].second ? !inputList[0].first : inputList[0].first;
    break;
  case CellSymbol::IN:
    assert(inputList.size() == 1);
    result = inputList[0].first;
    break;
  case CellSymbol::OUT:
    assert(inputList.size() == 1);
    result = inputList[0].second ? !inputList[0].first : inputList[0].first;
    break;
  case CellSymbol::NOT:
    assert(inputList.size() == 1);
    result = !inputList[0].first;
    break;
  case CellSymbol::AND:
    result = inputList[0].second ? !inputList[0].first : inputList[0].first;
    for (size_t i = 1; i < inputList.size(); i++) {
      secondOperand = inputList[i].second ?
                      !inputList[i].first :
                      inputList[i].first;
      result = result & secondOperand;
    }
    break;
  case CellSymbol::OR:
    result = inputList[0].second ? !inputList[0].first : inputList[0].first;
    for (size_t i = 1; i < inputList.size(); i++) {
      secondOperand = inputList[i].second ?
                      !inputList[i].first :
                      inputList[i].first;
      result = result | secondOperand;
    }
    break;
  case CellSymbol::XOR:
    result = inputList[0].second ? !inputList[0].first : inputList[0].first;
    for (size_t i = 1; i < inputList.size(); i++) {
      secondOperand = inputList[i].second ?
                      !inputList[i].first :
                      inputList[i].first;
      result = result ^ secondOperand;
    }
    break;
  case CellSymbol::NAND:
    result = inputList[0].second ? !inputList[0].first : inputList[0].first;
    for (size_t i = 1; i < inputList.size(); i++) {
      secondOperand = inputList[i].second ?
                      !inputList[i].first :
                      inputList[i].first;
      result = result & secondOperand;
    }
    result = !result;
    break;
  case CellSymbol::NOR:
    result = inputList[0].second ? !inputList[0].first : inputList[0].first;
    for (size_t i = 1; i < inputList.size(); i++) {
      secondOperand = inputList[i].second ?
                      !inputList[i].first :
                      inputList[i].first;
      result = result | secondOperand;
    }
    result = !result;
    break;
  case CellSymbol::XNOR:
    result = inputList[0].second ? !inputList[0].first : inputList[0].first;
    for (size_t i = 1; i < inputList.size(); i++) {
      secondOperand = inputList[i].second ?
                      !inputList[i].first :
                      inputList[i].first;
      result = result ^ secondOperand;
    }
    result = !result;
    break;
  // Only supports 3 arguments
  case CellSymbol::MAJ: {
    assert(inputList.size() == 3 &&
           "BDD converter only supports majority function of 3 arguments.");
    const BDD &x1 = inputList[0].second ?
                    !inputList[0].first :
                    inputList[0].first;
    const BDD &x2 = inputList[1].second ?
                    !inputList[1].first :
                    inputList[1].first;
    const BDD &x3 = inputList[2].second ?
                    !inputList[2].first :
                    inputList[2].first;
    result = (x1 & x2) | (x1 & x3) | (x2 & x3);
    break;
  }
  default:
    assert(false && "Unsupported gate");
    break;
  }
  return result;
}

void SubnetToBdd::convertAll(const Subnet &net,
                             SignedBDDList &outputSignedBDDList,
                             const Cudd &manager) {
  // Create a BDD variable with a corresponding index for each Subnet input
  CellBDDMap varMap;
  for (size_t i = 0; i < net.getInNum(); ++i) {
    varMap[i] = manager.bddVar(i);
  }

  SignedBDDList result(net.size());

  const auto &entries = net.getEntries();
  for (size_t i = 0; i < net.size(); ++i) {
    const auto &cell = entries[i].cell;

    std::pair<BDD, bool> resultBDD;
    if (cell.isIn()) {
        resultBDD = std::pair(varMap[i], false);
    } else {
      SignedBDDList inputsSignedBDDList;
      for (const auto &signal : net.getLinks(i)) {
        std::pair<BDD, bool> inputBDD = std::pair(result[signal.idx].first,
                                                  signal.inv);
        inputsSignedBDDList.push_back(inputBDD);
      }
      resultBDD = std::pair(applyGateFunc(cell.getSymbol(),
                                          inputsSignedBDDList,
                                          manager),
                            false);
    }
    result[i] = resultBDD;
    i += cell.more;
  }
  outputSignedBDDList = result;
}

BDD SubnetToBdd::convert(const Subnet &net,
                         unsigned cellId,
                         const Cudd &manager) {
  SignedBDDList resultList;
  convertAll(net, resultList, manager);
  assert(cellId < net.size() && "Gate index more than net size");
  return resultList[cellId].first;
}

BDDList SubnetToBdd::convertList(const Subnet &net,
                                 std::vector<unsigned> &list,
                                 const Cudd &manager) {
  SignedBDDList resultList;
  BDDList ret;
  convertAll(net, resultList, manager);

  for (const auto &i: list) {
    assert(i < net.size() && "Gate index more than net size");
    ret.push_back(resultList[i].first);
  }
  return ret;
}

}; // namespace eda::gate::model::utils
