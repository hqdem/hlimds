//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/net.h"
#include "gate/model/subnet.h"

/**
 * \brief Synthetic nets examples.
 */
namespace eda::gate::model {

SubnetID makeSubnet2AndOr();
NetID makeNet2AndOr();

SubnetID makeSubnet2AndOr2();
NetID makeNet2AndOr2();

SubnetID makeSubnet3AndOrXor();
NetID makeNet3AndOrXor();

SubnetID makeSubnetXorNorAndAndOr();
NetID makeNetXorNorAndAndOr();

SubnetID makeSubnetXorOrXor();
NetID makeNetXorOrXor();

SubnetID makeSubnetAndOrXor();
NetID makeNetAndOrXor();

SubnetID makeSubnet4AndOr();
NetID makeNet4AndOr();

SubnetID makeSubnet2Latches();
NetID makeNet2Latches();

SubnetID makeSubnetLatch();
NetID makeNetLatch();

SubnetID makeSubnetStuckLatches();
NetID makeNetStuckLatches();

SubnetID makeSubnetStuckLatch();
NetID makeNetStuckLatch();

SubnetID makeSubnetRandomMatrix(const size_t nIn,
                                const size_t nOut,
                                const size_t nCell,
                                const size_t minArity,
                                const size_t maxArity,
                                const unsigned seed);

NetID makeNetRandomMatrix(const size_t nIn,
                          const size_t nOut,
                          const size_t nCell, 
                          const size_t minArity,
                          const size_t maxArity,
                          const unsigned seed);

} // namespace eda::gate::model
