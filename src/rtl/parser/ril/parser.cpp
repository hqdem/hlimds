//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <stdio.h>

#include "rtl/parser/ril/parser.h"

// The parser is built w/ the prefix 'rr' (not 'yy').
extern FILE *rrin;
extern int rrparse(void);

namespace eda::rtl::parser::ril {

int parse(const std::string &filename) {
  FILE *file = fopen(filename.c_str(), "r");
  if (file == nullptr) {
    return -1;
  }

  rrin = file;
  return rrparse();
}

} // namespace eda::rtl::parser::rtl
