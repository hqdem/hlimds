//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#ifndef UNTITLED_METHODS_H
#define UNTITLED_METHODS_H
#include "gate/model/gate.h"
#include "gate/model/gnet.h"
#include "gate/model2/subnet.h"
#include "kitty/kitty.hpp"
#include <memory>
#include <vector>

namespace eda::gate::optimizer::resynthesis {

  /**
  * \brief Implements Cascade method of resynthesis.
  *
  * The algorithm based on the article "Method for the synthesis of 
  * computational and control contact circuits" by G. N. Povarov,
  * Avtomat i Telemekh., 1957, volume 18, issue 2, 145–162
  */
  class Cascade {

  public:

    //===------------------------------------------------------------------===//
    // Types
    //===------------------------------------------------------------------===//

    using CNF = std::vector<std::vector<int>>;
    using Gate = eda::gate::model::Gate;
    using GNet = eda::gate::model::GNet;
    using SignalList = Gate::SignalList;

    //===------------------------------------------------------------------===//
    // Constructors/Destructors
    //===------------------------------------------------------------------===//

    Cascade(kitty::dynamic_truth_table &table);

    //===------------------------------------------------------------------===//
    // Main Methods
    //===------------------------------------------------------------------===//

    /// Makes net using cascade method
    std::shared_ptr<GNet> run(SignalList &inputs, Gate::Id &outputId);

    /// Makes subnet using cascade method
    const auto &runSubnet();
    
    /// Makes CNF for function using cascade method
    CNF getFunction(kitty::dynamic_truth_table &table);

  private:

    kitty::dynamic_truth_table table;
    
    std::shared_ptr<GNet> net;

    std::vector<int> values;
    
    CNF form;
    
    //===----------------------------------------------------------------===//
    // Internal Methods
    //===----------------------------------------------------------------===//

    /// Initializes the cells of the vector
    void initialize(CNF &output, int times = 1, int num1 = 0, int num2 = 0, 
      int num3 = 0);

    /// Сalculates an expression when num_vars - 1 arguments are known
    int calculate(int numVars, CNF &form);

    /// Checks if out1 and out2 can be simplified and if so, does it;
    /// result is stored in out
    void checkSimplify(int, CNF &out, CNF &out1, CNF &out2);
                
    /// Calculates normal form
    static CNF normalForm(kitty::dynamic_truth_table &table);

  };
} // namespace eda::gate::optimizer::resynthesis
#endif
