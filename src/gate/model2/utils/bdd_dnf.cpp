//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "bdd_dnf.h"

namespace eda::gate::model::utils {

void collectPaths(DdNode *node,
                  BddToDnf::Paths &paths,
                  BddToDnf::Path &path,
                  bool targetConst = true) {
  /*
   * If the current node with negation, it is required to change
   * the constant to which the path is being searched for.
   */
  if (Cudd_IsComplement(node)) {
    targetConst = !targetConst;
  }

  DdNode *t = Cudd_T(node);
  DdNode *e = Cudd_E(node);

  if ((!Cudd_IsConstant(e) and Cudd_IsConstant(t))
      or (Cudd_IsConstant(e) and !Cudd_IsConstant(t))) {

    /*
     *          c            c            c            c
     *         / \          / \          / \          / \
     *       1/   \0      1/   *0      1/   \0      1/   *0
     *       /     \      /     \      /     \      /     \
     *      1       d    1       d    d       1    d       1
     *         c+d         c+!d         !c+d          c*d
     */

    if (Cudd_IsConstant(e)) {
      /*
       * Check the required sign:
       * If negation, then it is a zero-constant and must equal the target.
       * If there is no negation, then it is a one-constant.
       */
      if (targetConst == !Cudd_IsComplement(e)) {
        /*
         * Add the current variable with a minus because
         * the constant is to the right of the current node.
         */
        std::vector<std::pair<int, bool>> temp = path;
        temp.push_back({Cudd_NodeReadIndex(node), true});
        paths.push_back(temp);
      }
      // Left path decomposition, it is only positive.
      path.push_back({Cudd_NodeReadIndex(node), false});
      collectPaths(t, paths, path, targetConst);
      path.pop_back();
      return;
    }

    // The constant on the left is always one. If target is 1, add it.
    if (targetConst) {
      std::vector<std::pair<int, bool>> temp = path;
      temp.push_back({Cudd_NodeReadIndex(node), false});
      paths.push_back(temp);
    }

    path.push_back({Cudd_NodeReadIndex(node), true});
    collectPaths(e, paths, path, targetConst);
    path.pop_back();
    return;
  }

  if (Cudd_IsConstant(e) and Cudd_IsConstant(t)) {
    /*
     * Select with what sign to add the variable to the DNF
     * depending on the current target flag.
     */
    std::vector<std::pair<int, bool>> temp = path;
    if (targetConst) {
      temp.push_back({Cudd_NodeReadIndex(node), false});
    } else {
      temp.push_back({Cudd_NodeReadIndex(node), true});
    }
    paths.push_back(temp);
    return;
  }

  path.push_back({Cudd_NodeReadIndex(node), false});
  collectPaths(t, paths, path, targetConst);
  path.pop_back();

  path.push_back({Cudd_NodeReadIndex(node), true});
  collectPaths(e, paths, path, targetConst);
  path.pop_back();
  return;
}

void convertToCubes(BddToDnf::Paths &paths,
                    size_t varCount,
                    std::vector<kitty::cube> &ret) {

  for (auto vec: paths) {
    // Sorting a vector by modulus of numbers
    std::sort(vec.begin(), vec.end(),
              [](const std::pair<int, bool>& a, const std::pair<int, bool>& b)
    { return std::abs(a.first) < std::abs(b.first); });

    // String creation
    std::string result;
    for (int i = 0; i < varCount; i++) {
        result += "-";
    }
    for (auto pair : vec) {
      if (pair.second == true) {
        result[pair.first] = '0';
      } else {
        result[pair.first] = '1';
      }
    }

    // Creating a cube based on a string
    ret.push_back(result);
    LOG_DEBUG(result);
  }
}

std::vector<kitty::cube> BddToDnf::getDnf(BDD bdd) {
  std::vector<kitty::cube> ret;
  std::vector<std::vector<std::pair<int, bool>>> paths;
  std::vector<std::pair<int, bool>> path;
  collectPaths(bdd.getNode(), paths, path);
  convertToCubes(paths, Cudd_ReadSize(bdd.manager()), ret);
  return ret;
}

} // namespace eda::gate::model::utils
