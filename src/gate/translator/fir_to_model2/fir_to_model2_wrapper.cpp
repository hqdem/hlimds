//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "fir_to_model2_wrapper.h"

#include "fir_to_model2.h"
#include "gate/model2/printer/printer.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using Format = eda::gate::model::ModelPrinter::Format;

namespace fs = std::filesystem;

namespace eda::gate::model {

int translateToModel2(const std::string &inputFileName,
    const std::string &outputFileName,
    const InputFormat inputFormat) {
  // Parse the input 'FIRRTL' file.
  Translator translator{inputFormat == InputFormat::InputFIRFile ?
      MLIRModule::loadFromFIRFile(inputFileName) :
      MLIRModule::loadFromMLIRFile(inputFileName)};

  // Convert the 'FIRRTL' representation to the 'model2' representation.
  const auto resultNetlist = translator.translate();

  // Dump the output net to the '.dot' file.
  const fs::path outputFullName = outputFileName;
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