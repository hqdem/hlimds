//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/optimizer2/synthesis/cascade.h"
#include "kitty/kitty.hpp"

#include <cstring>
#include <memory>
#include <vector>

namespace eda::gate::optimizer2::synthesis {

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

using CNF = CascadeSynthesizer::CNF;
using Subnet = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID = model::SubnetID;
using TruthTable = kitty::dynamic_truth_table;

//===----------------------------------------------------------------------===//
// Constructors/Destructors
//===----------------------------------------------------------------------===//

CascadeSynthesizer::CascadeSynthesizer() {}

//===----------------------------------------------------------------------===//
// Internal Methods
//===----------------------------------------------------------------------===//

void CascadeSynthesizer::initialize(
    CNF &output, int times, int num1, int num2, int num3) const {

  for (int i = 0; i < times; i++) {
    output[0].push_back(num1);
    output[1].push_back(num2);
    output[2].push_back(num3);
  }
}

int CascadeSynthesizer::calculate(
    int numVars, CNF &form, std::vector<int> &values) const {

  // form[j][i] - current value in normal form table
  // values[j] : f(..., j, ...) - value of j
  // form[num_vars - 1][i] - last value in normal form table, x or !x

  int columnResult = 0;
  int result = 0;
  int size = form[0].size();
    
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < numVars - 1; j++) {
      if (!j) {
        columnResult = form[j][i] ? values[j] : !values[j];
      } else {
        if (form[j][i]) {
          columnResult &= values[j];
        } else if (values[j]) {
          columnResult = 0;
        }
      }
      if (!columnResult) {
        break;
      }
    }

    if (!i) {
      if (form[numVars - 1][i] && columnResult) {
        result = 2;
      } else if (columnResult) {
        result = 3;
      }
    } else {
      if (form[numVars - 1][i]) {
        if (columnResult) {
          result = (result == 2 || !result) ? 2 : 1;
        }
      } else {
        if (columnResult) {
          result = (result == 3 || !result) ? 3 : 1;
        }
      }
    }
    if (result == 1) {
      break;
    }
  }
  return result;
}

// Checks if x_i&f(1, ...) + !x_i&f(0, ...) can be simplified and if so, 
// does it
void CascadeSynthesizer::checkSimplify(
    int numVars, CNF &out, CNF &out1, CNF &out2, std::vector<int> &values) const {

  int size1 = out1[0].size();
  int size2 = out2[0].size();
  int valSize = values.size();
  int lastElem1 = out1[0][size1 - 1];
  int lastElem2 = out2[0][size2 - 1];
  int k = size1;
  // x_1/!x_1 ... x_n/!x_n, 0 and 1 columns
  int sourceInit = numVars * 2 + 2; 
  // 0, 1, x_1 ... x_n and !x_1 ... !x_(num of init values)
  int negValInit = numVars + 2 + valSize; 
  // 0, 1, x_1 ... x_(num of init values)
  int valInit = valSize + 2; 

  // f(1, ...) and f(0, ...) equality case
  if (out1[0] == out2[0] && out1[1] == out2[1] && out1[2] == out2[2]) {
    std::copy(out1.begin(), out1.end(), out.begin());
  } else if (lastElem1 == 1) {
    if (lastElem2 == 0) { // f(1, ...) == 1 and f(0, ...) == 0 case

      initialize(out, sourceInit);
      initialize(out, 1, valInit);

    } else { // f(1, ...) == 1 case
      std::copy(out2.begin(), out2.end(), out.begin());
      k = size2;
      initialize(out, 1, 2, k - 1, negValInit);
      k++;
      initialize(out, 1, 3, k - 1, valInit);

    }
  } else if (!lastElem1) { // f(1, ...) == 0 and f(0, ...) == 1 case
    if (lastElem2 == 1) {

      initialize(out, sourceInit);
      initialize(out, 1, negValInit);

    } else { // f(1, ...) == 0 case
      std::copy(out2.begin(), out2.end(), out.begin());
      k = size2;
      initialize(out, 1, 2, k - 1, negValInit);
    }
  } else {

    std::copy(out1.begin(), out1.end(), out.begin());
    initialize(out, 1, 2, k - 1, valInit);

    if (lastElem2 == 1) { // f(0, ...) == 1 case
      k++;
      initialize(out, 1, 3, k - 1, negValInit);
    } else if (lastElem2) { // Simplification is impossible
      int id = k;
      k++;

      for (int i = sourceInit; i < size2; i++) {

        initialize(out, 1, out2[0][i],
            out2[1][i] + size1 - sourceInit + 1, out2[2][i]);

        if (!out2[1][i] && !out2[2][i]) {
          out[1][k] = 0;	
        }

        if (out2[0][i] == 3 && out2[2][i] > (numVars * 2 + 1)) {
          out[2][k] += size1 - sourceInit + 1;
        }
        k++;
      }

      initialize(out, 1, 2, k - 1, negValInit);
      k++;
      initialize(out, 1, 3, k - 1, id);
    }
  }
}

CNF CascadeSynthesizer::normalForm(const TruthTable &table) const {

  int numVars = table.num_vars();
  int bits = 1 << numVars;

  CNF form(numVars);
  std::vector <TruthTable> variables;

  // creating table
  for (int i = 0; i < numVars; i++) {
    TruthTable a(numVars);
    kitty::create_nth_var(a, i);
    variables.push_back(a);
  }

  // creating normal form
  for (int i = bits - 1; i >= 0; i--) {
    if (get_bit(table, i)) {
      for (int j = 0; j < numVars; j++) {
        form[j].push_back(get_bit(variables[j], i));
      }
    }
  }
  return form;
}

//===--------------------------------------------------------------------===//
// Main Methods
//===--------------------------------------------------------------------===//

CNF CascadeSynthesizer::getFunction(
    const TruthTable &table, CNF &form, std::vector<int> &values) const {

  unsigned int numVars = table.num_vars();
  CNF output(3);

  if (kitty::count_zeros(table) == table.num_bits()) {
    initialize(output, numVars * 2 + 3);
    return output;
  }

  if (numVars == 1) {
    initialize(output, 4);
    if (kitty::count_ones(table) == table.num_bits()) {
      initialize(output, 1, 1);
    } else {
      initialize(output, 1, get_bit(table, 0) ? 2 : 3);
    }
    return output;
  }

  if (values.size() == numVars - 1) {

    int res = calculate(numVars, form, values);
    if (res == 2) {
      res = numVars + 1;
    } else if (res == 3) {
      res = numVars * 2 + 1;
    }

    initialize(output, numVars * 2 + 3);
    output[0][numVars * 2 + 2] = res;

    return output;
  }

  // double copying is used in x_i&f(1, ...) + !x_i&f(0, ...) to represent
  // f(1, ...) and f(0, ...) - stored in two different vectors
  values.push_back(1);
  CNF output1 = getFunction(table, form, values);
  values.pop_back();
  values.push_back(0);
  CNF output2 = getFunction(table, form, values);
  values.pop_back();
  checkSimplify(numVars, output, output1, output2, values);

  return output;
}

SubnetID CascadeSynthesizer::synthesize(
    const TruthTable &func, uint16_t maxArity) const {
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;
  const int undefinedArity = 65535;
  
  SubnetBuilder subnetBuilder;

  std::vector<int> values;
  CNF form = CascadeSynthesizer::normalForm(func);

  int numVars = func.num_vars();
  // id of the first value in the output line
  int firstValId = numVars * 2 + 2; 
  
  CNF output = CascadeSynthesizer::getFunction(func, form, values);

  int size = output[0].size();
  LinkList links;
  unsigned int InNum = size - 1; // number of cells in subnet

  std::vector<int> idx(InNum, 0);
  std::vector<bool> inverted(InNum, false);
  for (size_t i = 0; i < (size_t)numVars; ++i) {
    idx[i] = subnetBuilder.addInput().idx;
  }

  if (!output[0][size - 1]) { // 0 case
    idx[InNum - 2] = subnetBuilder.addCell(model::ZERO).idx;
    idx[InNum - 1] = subnetBuilder.addOutput(Link(idx[InNum - 2])).idx;

    return subnetBuilder.make();
  } 
  if (output[0][size - 1] == 1) { // 1 case
    idx[InNum - 2] = subnetBuilder.addCell(model::ONE).idx;
    idx[InNum - 1] = subnetBuilder.addOutput(Link(idx[InNum - 2])).idx;

    return subnetBuilder.make();
  }
  
  for (size_t i = numVars; i < (size_t)numVars * 2; i++) {
    idx[i] = idx[i - numVars]; // negotiation
    inverted[i] = true;
  }
  
  if (size == (3 + numVars * 2)) { // 1 source
    int idx1;
    int id = size - 1;
    if (output[0][id] >= (numVars + 2) && output[0][id] < (numVars * 2 + 2)) {
      idx1 = output[0][id] - 2 - numVars; // negative source
      idx[InNum - 1] = subnetBuilder.addOutput(~Link(idx[idx1])).idx;
    } else {
      idx1 = output[0][id] - 2; // non negative source
      idx[InNum - 1] = subnetBuilder.addOutput(Link(idx[idx1])).idx;
    }
    
    return subnetBuilder.make();
  }
  
  for (int i = firstValId; i < size; i++) { // building subnet
    if (!output[1][i] && !output[2][i]) { // one source case
      int idx1;
      if (output[0][i] >= (numVars + 2) && output[0][i] < (numVars * 2 + 2)) {
        idx1 = output[0][i] - 2 - numVars; // negative source
        inverted[i - 2] = true;
      } else {
        idx1 = output[0][i] - 2; // non negative source
      }
      idx[i - 2] = idx[idx1]; 
      if (output[1][i - 1] && output[2][i - 1]) {
        if (!links.empty() && output[0][i - 1] == 2) {
          idx[i - 3] = subnetBuilder.addCellTree(model::AND, links, maxArity).idx;
        } else if (output[0][i - 1] == 3) {
          idx[i - 3] = subnetBuilder.addCellTree(model::OR, links, maxArity).idx;
        }   
        links.clear();
      }
    } else { // two sources 
      int idx1 = output[1][i] - 2;
      int idx2 = output[2][i] - 2;
      
      const Link lhs(idx[idx1], inverted[idx1]);
      const Link rhs(idx[idx2], inverted[idx2]);

      // new cell
      if (maxArity == undefinedArity) {
        if (output[0][i] == 2) {
          idx[i - 2] = subnetBuilder.addCell(model::AND, lhs, rhs).idx; 
        } else if (output[0][i] == 3) {
          idx[i - 2] = subnetBuilder.addCell(model::OR, lhs, rhs).idx;
        }
      } else { // new cell with arity
        if (i != firstValId && output[0][i - 1] == output[0][i]) { // prev the same
          links.push_back(Link(idx[idx2], inverted[idx2])); // link added
        } else { // prev different
          if (!links.empty()) {
            if (output[0][i] == 2) {
              idx[i - 3] = subnetBuilder.addCellTree(model::OR, links, maxArity).idx; 
            } else if (output[0][i] == 3) {
              idx[i - 3] = subnetBuilder.addCellTree(model::AND, links, maxArity).idx;
            }
            links.clear();
          }
          links.push_back(Link(idx[idx1], inverted[idx1]));
          links.push_back(Link(idx[idx2], inverted[idx2]));
        }
      }
    }
  }
  
  if (!links.empty()) { 
    if (output[0][output[0].size() - 1] == 2) {
      idx[InNum - 2] = subnetBuilder.addCellTree(model::AND, links, maxArity).idx;
    } else if (output[0][output[0].size() - 1] == 3) {
      idx[InNum - 2] = subnetBuilder.addCellTree(model::OR, links, maxArity).idx;
    }
  }
  
  idx[InNum - 1] = subnetBuilder.addOutput(Link(idx[InNum - 2])).idx;
        
  return subnetBuilder.make();
}

} // namespace eda::gate::optimizer2::synthesis
