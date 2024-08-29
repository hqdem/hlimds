//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/synthesizer.h"

#include <memory>
#include <vector>

namespace eda::gate::optimizer::synthesis {

/**
* \brief Implements the cascade method.
*
* The implementation is based on the article "Method for the synthesis of 
* computational and control contact circuits" by G. N. Povarov,
* Avtomat i Telemekh., 1957, volume 18, issue 2, 145–162
*/
class CascadeSynthesizer : public TruthTableSynthesizer {
public:
  /// Conjunctive normal form
  using CNF = std::vector<std::vector<int>>;
  using SubnetObject = model::SubnetObject;
  using TruthTable = util::TruthTable;

  CascadeSynthesizer() {}

  using Synthesizer::synthesize;

  /// Synthesizes a subnet using the cascade method
  SubnetObject synthesize(const TruthTable &func, const TruthTable &,
                          uint16_t maxArity = -1) const override;
    
  /// Makes CNF for function using cascade method
  CNF getFunction(
      const TruthTable &table, CNF &form, std::vector<int> &values) const;

  /// Calculates normal form
  CNF normalForm(const TruthTable &table) const;

private:
  /// Initializes the cells of the vector
  void initialize(CNF &output, int times = 1, int num1 = 0, int num2 = 0, 
    int num3 = 0) const;

  /// Сalculates an expression when num_vars - 1 arguments are known
  int calculate(int numVars, CNF &form, std::vector<int> &values) const;

  /// Checks if out1 and out2 can be simplified and if so, does it;
  /// result is stored in out
  void checkSimplify(
      int, CNF &out, CNF &out1, CNF &out2, std::vector<int> &values) const;
};

} // namespace eda::gate::optimizer::synthesis
