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

static optimizer::NpnDatabase dbXag4;

/**
 * \brief Implements synthesis based on NPN4 database precomputed in XAG basis.
 */
class DbXag4Synthesizer : public DbSynthesizer,
                          public util::Singleton<DbXag4Synthesizer> {
public:
  friend class util::Singleton<DbXag4Synthesizer>;

  model::SubnetObject synthesize(const util::TTn &func,
                                 const util::TTn &,
                                 uint16_t) const override {

    return DbSynthesizer::synthesize(func, dbXag4);
  }

private:
  DbXag4Synthesizer() {
    const size_t k = 4;
    const auto xag4 = env::getHomePath() / "logdb" / "area_delay_xag4";
    dbXag4 = optimizer::NpnDatabase::importFrom(xag4);
    dbXag4.setInNum(k);
  }
};

} // namespace eda::gate::optimizer::synthesis
