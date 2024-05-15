//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/net.h"
#include "gate/translator/firrtl.h"

#include "llvm/Support/SourceMgr.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"

using CellID = eda::gate::model::CellID;
using CellTypeID = eda::gate::model::CellTypeID;
using NetID = eda::gate::model::NetID;
using MLIRContext = mlir::MLIRContext;
using ModuleOp = mlir::ModuleOp;
using Operation = mlir::Operation;
template<typename Type>
using OwningOpRef = mlir::OwningOpRef<Type>;
using Pass = mlir::Pass;
using PassManager = mlir::PassManager;

namespace eda::gate::model {

/**
 * @brief A key for identifying a cell in net needed for creating the links.
 * @author <a href="mailto:grigorovia@ispras.ru">Ivan Grigorov</a>
 */
struct CellKey {
  CellKey(Operation *operation,
          uint portNumber,
          uint bitNumber) :
      operation(operation),
      portNumber(portNumber),
      bitNumber(bitNumber) {}
  CellKey() : operation(nullptr), portNumber(0), bitNumber(0) {}
  bool operator==(const CellKey &cellKey) const;

  Operation *operation;
  uint portNumber;
  uint bitNumber;
};

} // namespace eda::gate::model

using CellKey = eda::gate::model::CellKey;

namespace std {

template<>
struct hash<CellKey> {
  size_t operator()(const CellKey &cellKey) const {
    size_t hash = std::hash<Operation*>()(cellKey.operation);
    hash = hash * 13 + std::hash<uint>()(cellKey.portNumber);
    hash = hash * 13 + std::hash<uint>()(cellKey.bitNumber);
    return hash;
  }
};

} // namespace std

namespace eda::gate::model {

/**
 * @brief A wrapper around a MLIR top operation (ModuleOp).
 * @author <a href="mailto:grigorovia@ispras.ru">Ivan Grigorov</a>
 */
class MLIRModule {
public:
  MLIRModule(MLIRModule &&oth);
  MLIRModule &operator=(MLIRModule &&oth);
  MLIRModule clone();
  MLIRContext *getContext();
  static MLIRModule loadFromMLIR(const std::string &string);
  static MLIRModule loadFromMLIRFile(const std::string &filename);
  static MLIRModule loadFromFIRFile(const std::string &filename);
  void print(llvm::raw_ostream &os);
  ModuleOp getRoot();

private:
  MLIRModule(std::shared_ptr<MLIRContext> context,
             mlir::OwningOpRef<ModuleOp> &&moduleOp);
  std::shared_ptr<MLIRContext> context;
  OwningOpRef<ModuleOp> moduleOp;
};

/**
 * @brief The top level module which does translation from 'FIRRTL' to 'model2'.
 * @author <a href="mailto:grigorovia@ispras.ru">Ivan Grigorov</a>
 */
class Translator {
public:
  Translator(MLIRModule &&module);
  void printFIRRTL();
  std::vector<CellTypeID> translate();

private:
  void addPass(std::unique_ptr<Pass> pass);
  void runPasses();
  void clearPasses();

  MLIRModule module;
  std::shared_ptr<std::vector<CellTypeID>> resultNetlist;
  PassManager passManager;
};

std::vector<CellTypeID> getModel2(const std::string &inputFilePath);

bool printNetlist(const std::string &inputFilePath,
                  const std::string &outputDir);
bool printNetlist(const std::vector<CellTypeID> netlist,
                  const std::string &outFileName);

} // namespace eda::gate::model

std::unique_ptr<Pass> createCHIRRTLToLowFIRRTLPass();
std::unique_ptr<Pass> createLowFIRRTLToModel2Pass(
    std::shared_ptr<std::vector<CellTypeID>> resultNetlist);
