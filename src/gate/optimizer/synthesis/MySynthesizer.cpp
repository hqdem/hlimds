#include "gate/optimizer/synthesis/MySynthesizer.h"
#include "gate/optimizer/synthesis/isop.h"
#include "util/kitty_utils.h"

#include <cmath>

namespace eda::gate::optimizer::synthesis {

// using Cube = kitty::cube;
// using Link = model::Subnet::Link;
// using LinkList = model::Subnet::LinkList;
// using SOP = std::vector<Cube>;
// using SubnetBuilder = model::SubnetBuilder;
// using SubnetObject = model::SubnetObject;

// using TruthTable = util::TruthTable;
using CubesList = std::vector<Cube>;

static std::pair<TruthTable, bool> handleCare(
    const TruthTable &func, const TruthTable &care) {
  if (care.num_vars() == 0) {
    bool inv{kitty::count_ones(func) > (func.num_bits() / 2)};
    return {inv ? ~func : func, inv};
  }
  auto funcWithCare{func & care};
  auto inverseFuncWithCare{(~func) & care};

  bool inv{kitty::count_ones(funcWithCare) > 
      kitty::count_ones(inverseFuncWithCare)};
    
  return {inv ? inverseFuncWithCare : funcWithCare, inv};
}

Link synthFromImplicant(Cube cube,
                        const LinkList &inputs,
                        SubnetBuilder &subnetBuilder,
                        uint16_t maxArity) {
    LinkList links;
    for (uint16_t idx = 0; idx < 32; ++idx) {
        if (cube.get_mask(idx) == 0) {
            continue;
        }
        bool inv = !(cube.get_bit(idx));
        links.push_back(Link(inputs[idx].idx, inv));
    }
    if (links.size() == 1) {
        return links[0];
    }
    return subnetBuilder.addCellTree(model::AND, links, maxArity);
}

Link synthFromImplicant_list(const CubesList &implicants,
                             const LinkList &inputs,
                             SubnetBuilder &subnetBuilder,
                             uint16_t maxArity) {
    LinkList links;
    for (uint16_t i = 0; i < implicants.size(); ++i) {
        const Link link = synthFromImplicant(implicants[i], inputs, subnetBuilder, maxArity);
        links.push_back(link);
    }
    if (links.size() == 1) {
        return links[0];
    }
    return subnetBuilder.addCellTree(model::OR, links, maxArity);
}

SubnetObject MySynthesizer::synthesize(const TruthTable &func,
                                       const TruthTable &care,
                                       uint16_t maxArity) const {
    SubnetObject object;
    SubnetBuilder &subnetBuilder(object.builder());
    LinkList ins(subnetBuilder.addInputs(func.num_vars()));

    const auto [tt, inv] = handleCare(func, care);

    if (bool value; util::isConst(tt, value)) {
        return SubnetBuilder::makeConst(tt.num_vars(), value ^ inv);
    }

    Link output(synthFromImplicant_list(kitty::get_prime_implicants_morreale(tt), ins, subnetBuilder, maxArity));
    subnetBuilder.addOutput(inv ? ~output : output);
    return object;
}
} // namespace eda::gate::optimizer::synthesis