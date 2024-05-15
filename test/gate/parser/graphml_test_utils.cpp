//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml_test_utils.h"
#include "test_util.h"

namespace eda::gate::parser::graphml {

SubnetID parse(std::string fileName, ParserData data) {
  using path = std::filesystem::path;

  fileName += ".bench.graphml";

  const path dir = path("test") / "data" / "gate" / "parser"
      / "graphml" / "OpenABC" / "graphml_openabcd";
  const path home = getHomePath();
  const path file = home / dir / fileName;

  uassert(std::filesystem::exists(file.string()),
                                  "File doesn't exist" << std::endl);

  GraphMlSubnetParser parser;
  return parser.parse(file.string(), data);
}

} // namespace eda::gate::parser::graphml
