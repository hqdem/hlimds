//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml_to_subnet.h"

#include <filesystem>
#include <string>

using GraphMlSubnetParser = eda::gate::parser::graphml::GraphMlSubnetParser;
using ParserData          = GraphMlSubnetParser::ParserData;
using Subnet              = GraphMlSubnetParser::Subnet;
using SubnetID            = eda::gate::model::SubnetID;

namespace eda::gate::parser::graphml {

SubnetID parse(std::string fileName, ParserData data);

} // namespace eda::gate::parser
