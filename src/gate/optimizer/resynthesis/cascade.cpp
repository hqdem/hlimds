//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/optimizer/resynthesis/cascade.h"
#include "kitty/kitty.hpp"
#include <memory>
#include <vector>

namespace eda::gate::optimizer::resynthesis {

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

  using CNF = Cascade::CNF;
  using GateSymbol = eda::gate::model::GateSymbol;
  using GNet = Cascade::GNet;

//===----------------------------------------------------------------------===//
// Internal Methods
//===----------------------------------------------------------------------===//

  void Cascade::initialize(CNF &output, int times, int num1, int num2, int num3) {

    for (int i = 0; i < times; i++) {
      output[0].push_back(num1);
      output[1].push_back(num2);
      output[2].push_back(num3);
    }
  }

  int Cascade::calculate(int numVars, CNF &form) {

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
  void Cascade::checkSimplify(int numVars, CNF &out, CNF &out1, CNF &out2) {

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

  CNF Cascade::normalForm(kitty::dynamic_truth_table &table) {

    int numVars = table.num_vars();
    int bits = 1 << numVars;
    // number of ones/zeroes between alternations in a table row
    // example: 1100, numOnes == 2; 1010, numOnes == 1;

    // numOnes and numOnesPrev are 2 copies of the same value
    // so that we can remember the value of the variable
    // on the previous iteration of the loop
    int numOnes = bits;
    int numOnesPrev = bits;
    CNF form(numVars);

    std::vector <kitty::dynamic_truth_table> variables;

    // creating table
    for (int i = 0; i < numVars; i++) {
      kitty::dynamic_truth_table a(numVars);
      variables.push_back(a);
      numOnes /= 2;

      for (int j = 0; j < bits; j++) {
        if (numOnesPrev && ((j%numOnesPrev) < numOnes)) {
          set_bit(variables[i], j);
        } else {
          clear_bit(variables[i], j);
        }
      }
      numOnesPrev = numOnes;
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

  CNF Cascade::getFunction(kitty::dynamic_truth_table &table) {

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

      int res = calculate(numVars, form);
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
    CNF output1 = getFunction(table);
    values.pop_back();
    values.push_back(0);
    CNF output2 = getFunction(table);
    values.pop_back();
    checkSimplify(numVars, output, output1, output2);

    return output;
  }

  std::shared_ptr<GNet> Cascade::run(SignalList &inputs, Gate::Id &outputId) {

    int numVars = table.num_vars();
    // id of the first value in the output line
    int firstValId = numVars * 2 + 2; 
    CNF output = Cascade::getFunction(table);

    Gate::Id id = net->addZero();
    output[0][0] = id;
    Gate::Signal input = Gate::Signal::always(id);
    inputs.push_back(input);
    id = net->addOne();
    output[0][1] = id;
    input = Gate::Signal::always(id);
    inputs.push_back(input);

    for (int i = 0; i < numVars; i++) {
      const Gate::Id id = net->addIn();
      output[0][i + 2] = id;
      const Gate::Signal input = Gate::Signal::always(id);
      inputs.push_back(input);
    }

    for (long unsigned int i = firstValId; i < output[0].size(); i++) {
      // output[j][i] - id of the column where the source is stored in 
      // the 0th row, so source is stored in the output[0][output[j][i]] 
      // cell in the negotiation case: (output[j][i] - numVars) is the id
      // subtraction is needed to take the source in its direct value:
      // (output[j][i]) is !x, so (output[j][i] - numVars) is x
      
      unsigned int source = output[0][output[0][i]];
      unsigned int source1 = output[0][output[1][i]];
      unsigned int source2 = output[0][output[2][i]];
      unsigned int negSource = output[0][output[0][i] - numVars];
      unsigned int negSource2 = output[0][output[2][i] - numVars];
      
      if (!output[1][i] && !output[2][i]) {
        if (output[0][i] < numVars + 2) {
          output[0][i] = source;
        } else if (!source) {
          const Gate::Id id = net->addNot(negSource);
          output[0][output[0][i]] = id;
          output[0][i] = id;

        } else if (source) {
          output[0][i] = source;
        }
      } else {
        if (output[2][i] < firstValId &&
            output[2][i] > numVars + 1 && !source2) {
          const Gate::Id id = net->addNot(negSource2);
          output[0][output[2][i]] = id;
        }
        Gate::Id id = 0;
        if (output[0][i] == 2) {
          id = net->addAnd(source1, source2);
        } else if (output[0][i] == 3) {
          id = net->addOr(source1, source2);
        }
        output[0][i] = id;
      }
    }
    outputId = net->addOut(output[0][output[0].size() - 1]);
    net->sortTopologically();
    return net;
  }

  const auto &runSubnet() {

    using Link = Subnet::Link;
    SubnetBuilder subnetBuilder;

    int numVars = table.num_vars();
    // id of the first value in the output line
    int firstValId = numVars * 2 + 2; 
    CNF output = Cascade::getFunction(table);
    int size = output[0].size();
    unsigned int InNum = size - 1; // number of cells in subnet

    size_t idx[InNum];
    for (size_t i = 0; i < InNum; ++i) {
      idx[i] = subnetBuilder.addCell(IN, SubnetBuilder::INPUT);
    }

    if (!output[0][size - 1]) { // 0 case
      idx[InNum - 2] = subnetBuilder.addCell(ZERO, SubnetBuilder::INPUT);
      idx[InNum - 1] = subnetBuilder.addCell(OUT, Link(idx[InNum - 2]), 
          SubnetBuilder::OUTPUT);

      const auto &subnet = Subnet::get(subnetBuilder.make());
      return subnet;

    } else if (output[0][size - 1] == 1) { // 1 case
      idx[InNum - 2] = subnetBuilder.addCell(ONE, SubnetBuilder::INPUT);
      idx[InNum - 1] = subnetBuilder.addCell(OUT, Link(idx[InNum - 2]), 
          SubnetBuilder::OUTPUT);

      const auto &subnet = Subnet::get(subnetBuilder.make());
      return subnet;
    }

    for (int i = numVars; i < numVars * 2; i++) { // negotiation
      const Link link(idx[i - numVars]); // source
      idx[i] = subnetBuilder.addCell(NOT, link);
    }

    for (int i = firstValId; i < size; i++) { // building subnet
      if (!output[1][i] && !output[2][i]) { // one source case
        int idx1 = output[0][i] - 2; // link to source
        idx[i - 2] = idx[idx1]; 
      } else { // two sources 
        int idx1 = output[1][i] - 2;
        int idx2 = output[2][i] - 2;

        const Link lhs(idx[idx1]); // link to 1st source
        const Link rhs(idx[idx2]); // link to 2nd source

        // new cell
        if(output[0][i] == 2) {
          idx[i - 2] = subnetBuilder.addCell(AND, lhs, rhs);
          k++;
        } else if (output[0][i] == 3) {
          idx[i - 2] = subnetBuilder.addCell(OR, lhs, rhs);
          k++;
        }
      }
    }
    idx[InNum - 1] = subnetBuilder.addCell(OUT, Link(idx[InNum - 2]), 
        SubnetBuilder::OUTPUT);
    const auto &subnet = Subnet::get(subnetBuilder.make());

    return subnet;
  }

//===----------------------------------------------------------------------===//
// Constructors/Destructors
//===----------------------------------------------------------------------===//

  Cascade::Cascade(kitty::dynamic_truth_table &_table) : table(_table), 
    net(std::make_shared<GNet>()), form(normalForm(table)) {}
}; // namespace eda::gate::optimizer::resynthesis
