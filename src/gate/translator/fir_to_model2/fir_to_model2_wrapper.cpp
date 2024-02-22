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

int translateToModel2(const FirrtlConfig &firrtlConfig,
                      const Model2Config &model2Config) {
  fs::path inputFilePath = model2Config.files.back();
  const std::string extension = inputFilePath.extension();
  if (extension != ".sv" && extension != ".v" && extension != ".fir") {
    std::cerr << "The input files are not supported! Abort." << std::endl;
    return 1;
  }
  if (!(model2Config.files.size() == 1 && extension == ".fir")) {
    bool areAllVerilog = true;
    for (const auto &file : firrtlConfig.files) {
      const fs::path filePath = file;
      std::string extension = filePath.extension();
      if (extension != ".sv" && extension != ".v") {
        areAllVerilog = false;
      }
    }
    if (areAllVerilog) {
      if (!firrtlConfig.outputNamefile.empty()) {
        if (!translateToFirrtl(firrtlConfig)) {
          inputFilePath = firrtlConfig.outputNamefile;
        } else {
          return 1;
        }
      } else {
        std::cerr << "The output file name is missing! Abort." << std::endl;
        return 1;
      }
    } else {
      std::cerr << "The input files are not supported! Abort." << std::endl;
      return 1;
    }
  }
  // Parse the input 'FIRRTL' file.
  Translator translator{ MLIRModule::loadFromFIRFile(inputFilePath.c_str()) };

  // Convert the 'FIRRTL' representation to the 'model2' representation.
  const auto resultNetlist = translator.translate();

  // Print the resulting 'model2' representation.
  for (const auto &cellTypeID : *resultNetlist) {
    std::cout << CellType::get(cellTypeID).getNet() << std::endl;
  }
  // Dump the output net to the '.v' file.
  if (!model2Config.outNetFileName.empty()) {
    const fs::path outputFullName = model2Config.outNetFileName;
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
  }

  return 0;
}
} // namespace eda::gate::model