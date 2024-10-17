//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/function/truth_table.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/npndb.h"
#include "gate/optimizer/synthesizer.h"

#include <filesystem>
#include <string>

#pragma once

namespace eda::gate::optimizer::synthesis {

/**
 * \brief Interface for synthesizers based on searching of precomputed Subnets
 *  in the databases.
 */
class DbSynthesizer : virtual public TruthTableSynthesizer {
public:
  using SubnetObject = gate::model::SubnetObject;
  using Synthesizer::synthesize;

  virtual ~DbSynthesizer() = default;

  SubnetObject synthesize(const model::TTn &func, NpnDatabase &db) const {
    const auto &iter = db.get(func);
    if (iter.isEnd()) {
      return SubnetObject();
    }
    SubnetObject object(iter.get());
    return object;
  }

};

} // namespace eda::gate::optimizer::synthesis
