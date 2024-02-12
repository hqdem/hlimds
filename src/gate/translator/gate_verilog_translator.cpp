//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/gate_verilog_translator.h"

#include "gate/model2/printer/printer.h"
#include "gate/translator/fir_to_model2/fir_to_model2.h"
#include "gate/translator/firrtl.h"

#include "options.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using Format = eda::gate::model::ModelPrinter::Format;

namespace fs = std::filesystem;

namespace eda::gate::model {

int translateToGateVerilog(
    const std::string &inputFileName,
    const TranslatorOptions &gateVerilog) {
  fs::path firFileName = inputFileName;
  firFileName.replace_extension(".fir");

  FirrtlConfig cfg;
  cfg.debugMode = false;
  cfg.outputNamefile = firFileName.string();
  cfg.files.push_back(inputFileName);
  translateToFirrtl(cfg);

  Translator translator{ MLIRModule::loadFromFIRFile(firFileName) };

  // Convert the 'FIRRTL' representation to the 'model2' representation.
  const auto resultNetlist = translator.translate();

  // Dump the output net to the '.dot' file.
  const fs::path outputFullName = gateVerilog.outFileName;
  const fs::path outputFullPath = outputFullName.parent_path();
  if (!outputFullPath.empty()) {
    fs::create_directories(outputFullPath);
  }
  std::ofstream outputStream(outputFullName);
  for (const auto &cellTypeID : *resultNetlist) {
    ModelPrinter::getPrinter(Format::VERILOG).print(outputStream,
        CellType::get(cellTypeID).getNet());
  }
  outputStream.close();
  return 0;
}
} // namespace eda::gate::model
