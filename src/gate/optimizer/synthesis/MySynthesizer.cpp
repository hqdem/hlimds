#include "gate/optimizer/synthesis/myrf.h"
#include "gate/optimizer/synthesis/isop.h"
#include "util/kitty_utils.h"

#include <cmath>

z`
namespace eda::gate::optimizer::synthesis


Link synthFromImplicant_list(const CubesList &implicants,
                             const linklist &inputs,
                             SubnetBuilder &subnetBuilder,
                             uint16_t maxArity) {
    LinkList links;
    for (uint16_t i = 0; i < implicants.size(); ++i) {
        const link link = synthFromImplicant(implicants[i], inputs, subnetBuilder, maxArity);
        links.push_back(link);
    }
    if (links.size() == 1) {
        return links[0];
    }
    return subnetBuilder.addCellTree(model::OR, links, maxArity);
}

Link synthFromImplicant(cube cube,
                        const linklist &inputs,
                        SubnetBuilder &subnetBuilder,
                        uint16_t maxArity) {
    LinkList links;
    for (uint16_t idx = 0; idx < cube.size(); ++idx) {
        if (cube.get_mask(idx) == 0) {
            continue;
        }
        bool inv = !(cube.get_bit(idx));
        links.push_back(link(inputs[idx], inv));
    }
    if (links.size() == 1) {
        return links[0];
    }
    return subnetBuilder.addCellTree(model::AND, links, maxArity);
}

SubnetObject MySynthesizer::synthesize(const TruthTable &func,
                                       const TruthTable &care,
                                       uint16_t maxArity) const {
    SubnetObject object;
    SubnetBuilder &subnetBuilder(object.builder());
    LinkList ins(subnetBuilder.addInputs(func.num_vars()));

    const auto [tt, inv] = handleXor(func, care);

    if (bool value; util::isConst(tt, value)) {
        return subnetBuilder.makeConst(ins[0].num_vars(), value ^ inv);
    }

    Link output(synthFromImplicant_list(kitty::get_prime_implicants_morreale(tt), ins, subnetBuilder, maxArity));
    subnetBuilder.addOutput(inv ? ~output : output);
    return object;
}
