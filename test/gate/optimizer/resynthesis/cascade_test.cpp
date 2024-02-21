//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/optimizer/resynthesis/cascade.h"
#include "gate/optimizer/rwdatabase.h"
#include "gate/optimizer/truthtable.h"

#include "gtest/gtest.h"

#include <kitty/kitty.hpp>

#include <memory>
#include <string>

namespace eda::gate::optimizer::resynthesis {
  
  using BGNet = gate::optimizer::BoundGNet;
  using CNF = std::vector<std::vector<int>>;
  using Gate = eda::gate::model::Gate;
  using SignalList = Gate::SignalList;
  using TruthTable = eda::gate::optimizer::TruthTable;
  
//===----------------------------------------------------------------------===//
// Convenience Methods
//===----------------------------------------------------------------------===//

  bool areEqualTT(kitty::dynamic_truth_table &table, TruthTable &tt) {
    for (size_t i = 0; i < table.num_bits(); i++) {
      bool bit = (tt.raw() >> i) & 1;
      if (kitty::get_bit(table, i) != bit) {
        return false;
      }
    }
    return true;
  }
  
  /// Transforms output into string
  kitty::dynamic_truth_table checkSynth(int numVars, int bits, CNF &output) {

    int numOnes = bits, numOnesPrev = bits; // creating table

    std::vector<kitty::dynamic_truth_table> result;

    for (int i = 0; i < 2; i++) {
      kitty::dynamic_truth_table a(numVars);
      result.push_back(a);
      for (int j = 0; j < bits; j++) {
        if (!i) {
          clear_bit(result[i], j);
        } else {
          set_bit(result[i], j);
        }
      }
    }
    for (int i = 2; i < numVars + 2; i++) {
      kitty::dynamic_truth_table a(numVars);
      result.push_back(a);
      numOnes /= 2;

      for (int j = 0; j < bits; j++) {
        if ((j % numOnesPrev) < numOnes) {
          set_bit(result[i], j);
        } else {
          clear_bit(result[i], j);
        }
      }
      numOnesPrev = numOnes;
    }
  
    numOnes = bits, numOnesPrev = bits;
    for (int i = numVars + 2; i < numVars * 2 + 2; i++) {
      kitty::dynamic_truth_table a(numVars);
      result.push_back(a);
      numOnes /= 2;

      for (int j = 0; j < bits; j++) {
        if ((j % numOnesPrev) < numOnes) {
          clear_bit(result[i], j);
        } else {
          set_bit(result[i], j);
        }
      }
      numOnesPrev = numOnes;
    }

    for (long unsigned int i = numVars * 2 + 2; i < output[0].size(); i++) {
      if (!output[1][i] && !output[2][i]) { // constant
        int id = output[0][i];
        result.push_back(result[id]);
      } else if (output[0][i] == 2) { // &
        int id1 = output[1][i];
        int id2 = output[2][i];
        kitty::dynamic_truth_table tt = result[id1];
        tt &= result[id2];
        result.push_back(tt);
      } else if (output[0][i] == 3) { // |
        int id1 = output[1][i];
        int id2 = output[2][i];
        kitty::dynamic_truth_table tt = result[id1];
        tt |= result[id2];
        result.push_back(tt);
      }
    }
    return result[result.size() - 1];
  }

  /// Checks if gnet and input string are equal
  void gnetTest(int vars, std::string str) {
    kitty::dynamic_truth_table table(vars);
    kitty::create_from_binary_string(table, str);

    Gate::SignalList inputs;
    Gate::Id outputId;
    BGNet bGNet;

    Cascade obj(table);
    bGNet.net = obj.run(inputs, outputId);
    bGNet.inputBindings = {inputs[0].node(), inputs[1].node(), 
        inputs[2].node(), inputs[3].node(), inputs[4].node(), inputs[5].node()};
    bGNet.outputBindings = {outputId};
    TruthTable gNetTT = TruthTable::build(bGNet);

    EXPECT_TRUE(areEqualTT(table, gNetTT));
  }

  /// Checks if input string and output are equal
  void synthTest(int vars, std::string str) {
    kitty::dynamic_truth_table table(vars);
    kitty::create_from_binary_string(table, str);

    Cascade obj(table);
    CNF output = obj.getFunction(table);
    kitty::dynamic_truth_table tt(vars);
    tt = checkSynth(vars, table.num_bits(), output);
    
    EXPECT_TRUE(tt == table);
  }

  TEST(Cascade, CorrectSynthTest) {
    synthTest(4, "1001000000100100");
  }
  
}; // namespace eda::gate::optimizer::resynthesis
