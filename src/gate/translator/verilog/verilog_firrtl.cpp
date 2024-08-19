//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "verilog_firrtl.h"

#include "gate/model/printer/printer.h"
#include "gate/translator/firrtl/firrtl_net.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using CellType = eda::gate::model::CellType;
using Format = eda::gate::model::ModelPrinter::Format;
using ModelPrinter = eda::gate::model::ModelPrinter;

namespace fs = std::filesystem;

namespace eda::gate::translator {

int translateVerilogFIRRTL(const FirrtlConfig &firrtlConfig) {
  fs::path inputFilePath = firrtlConfig.files.back();
  fs::path outputFilePath = firrtlConfig.outputFileName;
  const std::string extension = inputFilePath.extension();
  if (extension != ".v" && extension != ".fir") {
    std::cerr << "Unsupported file type: " << extension << "\n";
    return false;
  }
  if (!(firrtlConfig.files.size() == 1 && extension == ".fir")) {
    for (const auto &file : firrtlConfig.files) {
      const fs::path filePath = file;
      std::string extension = filePath.extension();
      if (extension != ".sv" && extension != ".v") {
        std::cerr << "The input files are not supported!\n";
        return false;
      }
    }
    if (!firrtlConfig.outputFileName.empty()) {
      const fs::path outputFullPath = outputFilePath.parent_path();
      if (!outputFullPath.empty()) {
        fs::create_directories(outputFullPath);
      }
      if (!translateToFirrtl(firrtlConfig)) {
        inputFilePath = firrtlConfig.outputFileName;
      } else {
        return false;
      }
    } else {
      std::cerr << "The output file name is missing!\n";
      return false;
    }
  }
  // Parse the input 'FIRRTL' file.
  Translator translator{ MLIRModule::loadFromFIRFile(inputFilePath.c_str()) };

  // Convert the 'FIRRTL' representation to the 'model2' representation.
  const auto resultNetlist = translator.translate();

  // Print the resulting 'model2' representation.
#ifdef UTOPIA_DEBUG
  for (const auto &cellTypeID : resultNetlist) {
    std::cout << CellType::get(cellTypeID).getNet() << "\n";
  }
#endif // UTOPIA_DEBUG

  // Dump the output net to the '.v' file.
  if (firrtlConfig.debugMode) {
    if (firrtlConfig.outputFileName.empty()) {
      std::cerr << "The output file name is missing!\n";
    }
    outputFilePath.replace_extension(".v");
    std::ofstream outputStream(outputFilePath);
    for (const auto &cellTypeID : resultNetlist) {
      ModelPrinter::getPrinter(Format::VERILOG).print(outputStream,
          CellType::get(cellTypeID).getNet());
    }
    outputStream.close();
  }

  return true;
}

} // namespace eda::gate::translator