//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"

#include <string>

namespace eda::gate::parser::verilog {

  /**
    * \brief Parses Verilog file and constructs the net.
    * @param netName Name of Verilog file.
    * @return The constructed net.
    */
  eda::gate::model::GNet *parseVerilog(const std::string &infile);

} // namespace eda::gate::parser::verilog
