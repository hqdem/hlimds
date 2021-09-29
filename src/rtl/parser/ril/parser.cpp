/*
 * Copyright 2021 ISP RAS (http://www.ispras.ru)
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */

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
