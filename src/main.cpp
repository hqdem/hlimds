//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "config.h"

#include "gate/model/gate.h"
#include "gate/model/netlist.h"
#include "hls/model/model.h"
#include "hls/parser/hil/builder.h"
#include "hls/parser/hil/parser.h"
#include "rtl/compiler/compiler.h"
#include "rtl/library/flibrary.h"
#include "rtl/model/net.h"
#include "rtl/parser/ril/builder.h"
#include "rtl/parser/ril/parser.h"
#include "util/utils.h"

using namespace eda::gate::model;
using namespace eda::rtl::compiler;
using namespace eda::rtl::library;
using namespace eda::rtl::model;
using namespace eda::utils;

int rtl_main(const std::string &filename) {
  if (eda::rtl::parser::ril::parse(filename) == -1) {
    std::cout << "Could not parse " << filename << std::endl;
    std::cout << "Synthesis terminated." << std::endl;
    return -1;
  }

  std::unique_ptr<Net> pnet = eda::rtl::parser::ril::Builder::get().create();
  pnet->create();

  std::cout << "------ p/v-nets ------" << std::endl;
  std::cout << *pnet << std::endl;

  Compiler compiler(FLibraryDefault::get());
  std::unique_ptr<Netlist> netlist = compiler.compile(*pnet);

  std::cout << "------ netlist ------" << std::endl;
  std::cout << *netlist;

  return 0;
}

int hls_main(const std::string &filename) {
  if (eda::hls::parser::hil::parse(filename) == -1) {
    std::cout << "Could not parse " << filename << std::endl;
    std::cout << "Synthesis terminated." << std::endl;
    return -1;
  }

  std::unique_ptr<Model> model = eda::hls::parser::hil::Builder::get().create();
  std::cout << *model;

  return 0;
}

int main(int argc, char **argv) {
  std::cout << "EDA Utopia ";
  std::cout << VERSION_MAJOR << "." << VERSION_MINOR << " | ";
  std::cout << "Copyright (c) 2021 ISPRAS" << std::endl;

  if (argc <= 1) {
    std::cout << "Usage: " << argv[0] << " <input-file(s)>" << std::endl;
    std::cout << "Synthesis terminated." << std::endl;
    return -1;
  }

  int result = 0;
  for (int i = 1; i < argc; i++) {
    const std::string filename = argv[i];

    int status = -1;
    if (ends_with(filename, ".ril")) {
      status = rtl_main(filename);
    } else if (ends_with(filename, ".hil")) {
      status = hls_main(filename);
    } else {
      std::cout << "Unknown format: " << filename << std::endl;
      status = -1;
    }

    result = (result == 0 ? status : result);
  }

  return result;
}

