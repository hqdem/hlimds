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

static optimizer::NpnDatabase dbMig4;

/**
 * \brief Implements synthesis based on NPN4 database precomputed in MIG basis.
 */
class DbMig4Synthesizer : public DbSynthesizer,
                          public util::Singleton<DbMig4Synthesizer> {
public:
  friend class util::Singleton<DbMig4Synthesizer>;

  model::SubnetObject synthesize(const util::TTn &func,
                                 const util::TTn &,
                                 uint16_t) const override {

    return DbSynthesizer::synthesize(func, dbMig4);
  }

private:
  DbMig4Synthesizer() {
    const size_t k = 4;
    const auto mig4 = env::getHomePath() / "logdb" / "percy_akers_mig4";
    dbMig4 = optimizer::NpnDatabase::importFrom(mig4);
    dbMig4.setInNum(k);
  }
};

} // namespace eda::gate::optimizer::synthesis
