#pragma once

#include <kitty/kitty.hpp>
#include <utility>
#include <vector>

#include "gate/model/subnet.h"
#include "gate/optimizer/synthesis/algebraic_factor.h"
#include "gate/optimizer/synthesizer.h"
#include "util/kitty_utils.h"

namespace eda::gate::optimizer::synthesis {

/// @cond ALIASES
using Cube = kitty::cube;
using Link = model::Subnet::Link;
using LinkList = model::Subnet::LinkList;
using SOP = std::vector<Cube>;
using SubnetBuilder = model::SubnetBuilder;
using SubnetObject = model::SubnetObject;
using TruthTable = util::TruthTable;
/// @endcond

class MySynthesizer final : public TruthTableSynthesizer {
 public:
  using TruthTableSynthesizer::synthesize;

  SubnetObject synthesize(const TruthTable &func, const TruthTable &care,
                          uint16_t maxArity = -1) const override;
};

}  // namespace eda::gate::optimizer::synthesis
