//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml_parser.h"

#include <filesystem>
#include <string>

using GraphMlParser = eda::gate::parser::graphml::GraphMlParser;
using ParserData    = GraphMlParser::ParserData;
using SubnetBuilder = GraphMlParser::SubnetBuilder;
using SubnetID      = eda::gate::model::SubnetID;

namespace eda::gate::parser::graphml {

std::shared_ptr<SubnetBuilder> parse(const std::string &fileName,
                                     ParserData *data = nullptr);

} // namespace eda::gate::parser
