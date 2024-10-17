//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/synthesis/db_synthesizer.h"
#include "util/env.h"

#include <filesystem>

#pragma once

namespace eda::gate::optimizer::synthesis {

static optimizer::NpnDatabase dbAig4;

/**
 * \brief Implements synthesis based on NPN4 database precomputed in AIG basis.
 */
class DbAig4Synthesizer : public DbSynthesizer,
                          public util::Singleton<DbAig4Synthesizer> {
public:
  friend class util::Singleton<DbAig4Synthesizer>;

  model::SubnetObject synthesize(const model::TTn &func,
                                 const model::TTn &,
                                 uint16_t) const override {

    return DbSynthesizer::synthesize(func, dbAig4);
  }

private:
  DbAig4Synthesizer() {
    const size_t k = 4;
    const auto aig4 = env::getHomePath() / "logdb" / "aig4";
    dbAig4 = optimizer::NpnDatabase::importFrom(aig4);
    dbAig4.setInNum(k);
  }
};

} // namespace eda::gate::optimizer::synthesis
