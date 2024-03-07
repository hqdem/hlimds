//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"

#include <memory>
#include <string>

namespace eda::gate::parser {
  using GNet = eda::gate::model::GNet;

  // Supported HDL.
  enum Exts {
    RIL,
    VERILOG,
    UNSUPPORTED,
  };

  /**
    * \brief Parses Verilog file from the project test suite and builds the net.
    * @param netName Name of Verilog file.
    * @return The constructed net.
    */
  GNet *parseVerilogTest(const std::string &infile);

  /**
    * \brief Parses RIL file and constructs the net.
    * @param fileName Name of RIL file.
    * @param subPath Sub-path to the file.
    * @return The constructed net.
    */
  std::unique_ptr<GNet> parseRil(const std::string &fileName,
                                 const std::string &subPath);

  /**
   * \brief Parses input description & builds net.
   * @param fileName Name of the file.
   * @param subPath Relative path to the file.
   * @return The parsed net.
   */
  GNet getModel(const std::string &fileName,
                const std::string &subPath);

  /**
   * \brief Finds out the extension of the file.
   * @param fileName Name of the file.
   * @return The extension, if it is supported, error otherwise.
   */
  Exts getExt(const std::string &fileName);

} // namespace eda::gate::parser
