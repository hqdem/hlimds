//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml_test_utils.h"

namespace eda::gate::parser::graphml {

SubnetID parse(std::string fileName, ParserData data) {
  uassert(getenv("UTOPIA_HOME"), "UTOPIA_HOME is not set" << std::endl);

  using path = std::filesystem::path;

  fileName += ".bench.graphml";

  const path dir = path("test") / "data" / "gate" / "parser"
      / "graphml" / "OpenABC" / "graphml_openabcd";
  const path home = std::string(getenv("UTOPIA_HOME"));
  const path file = home / dir / fileName;

  uassert(std::filesystem::exists(file.string()),
                                  "File doesn't exist" << std::endl);

  GraphMlSubnetParser parser;
  return parser.parse(file.string(), data);
}

}
