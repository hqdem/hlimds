//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/synthesis/db_mig4_synthesizer.h"
#include "gate/optimizer/synthesis/db_xag4_synthesizer.h"
#include "gate/optimizer/synthesis/isop.h"
#include "gate/premapper/cell_aigmapper.h"
#include "gate/premapper/cell_migmapper.h"
#include "gate/premapper/cell_xagmapper.h"
#include "gate/premapper/cell_xmgmapper.h"
#include "gate/premapper/cone_premapper.h"

namespace eda::gate::premapper {

inline optimizer::SubnetMapper getCellAigMapper() {
  return std::make_shared<CellAigMapper>("aig");
}

inline optimizer::SubnetMapper getCellMigMapper() {
  return std::make_shared<CellMigMapper>("mig");
}

inline optimizer::SubnetMapper getCellXagMapper() {
  return std::make_shared<CellXagMapper>("xag");
}

inline optimizer::SubnetMapper getCellXmgMapper() {
  return std::make_shared<CellXmgMapper>("xmg");
}

inline optimizer::SubnetMapper getConeAigMapper() {
  static optimizer::synthesis::MMFactorSynthesizer isop;
  static optimizer::Resynthesizer resynthesizer(isop);
  return std::make_shared<ConePremapper>("aig", AIG, resynthesizer, 4);
}

inline optimizer::SubnetMapper getConeMigMapper() {
  static optimizer::Resynthesizer resynthesizer(
      optimizer::synthesis::DbMig4Synthesizer::get());
  return std::make_shared<ConePremapper>("mig", MIG, resynthesizer, 4);
}

inline optimizer::SubnetMapper getConeXagMapper() {
  static optimizer::Resynthesizer resynthesizer(
      optimizer::synthesis::DbXag4Synthesizer::get());
  return std::make_shared<ConePremapper>("xag", XAG, resynthesizer, 4);
}

inline optimizer::SubnetMapper getConeXmgMapper() {
  static optimizer::Resynthesizer resynthesizer(
      optimizer::synthesis::DbMig4Synthesizer::get());
  return std::make_shared<ConePremapper>("xmg", XMG, resynthesizer, 4);
}

} // namespace eda::gate::premapper
