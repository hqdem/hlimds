//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"

/**
 * \brief Synthetic nets examples.
 */
namespace eda::gate::model {

model::SubnetID make2AndOr();

model::SubnetID make2AndOr2();

model::SubnetID make3AndOrXor();

model::SubnetID makeXorNorAndAndOr();

model::SubnetID makeXorOrXor();

model::SubnetID makeAndOrXor();

model::SubnetID make4AndOr();

model::SubnetID make2Latches();

model::SubnetID makeLatche();

model::SubnetID makeStuckLatches();

model::SubnetID makeStuckLatche();

model::SubnetID makeRandomSubnetMatrix(const size_t nIn,
                                       const size_t nOut,
                                       const size_t nCell,
                                       const size_t minArity,
                                       const size_t maxArity,
                                       const unsigned seed);

} // namespace eda::gate::model
