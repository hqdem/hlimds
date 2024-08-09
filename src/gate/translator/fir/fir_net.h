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
#include "util/hash.h"

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

namespace eda::gate::translator {

/**
 * @brief A key for identifying a link in a net.
 * @author <a href="mailto:grigorovia@ispras.ru">Ivan Grigorov</a>
 */
struct LinkKey final {
  LinkKey(Operation *op,
          const uint portNum,
          const uint bitNum) :
      op(op),
      portNum(portNum),
      bitNum(bitNum) {}
  LinkKey() : op(nullptr), portNum(0), bitNum(0) {}
  bool operator==(const LinkKey &cellKey) const;

  Operation *op;
  const uint portNum;
  const uint bitNum;
};

/**
 * @brief A key for identifying a cell type in a net.
 * @author <a href="mailto:grigorovia@ispras.ru">Ivan Grigorov</a>
 */
struct CellTypeKey final {
  CellTypeKey(const std::string &name,
              const std::vector<uint16_t> portWidthIn,
              const std::vector<uint16_t> portWidthOut) :
      name(name),
      portWidthIn(portWidthIn),
      portWidthOut(portWidthOut) {}
  CellTypeKey() : name(""), portWidthIn(0), portWidthOut(0) {}
  bool operator==(const CellTypeKey &cellTypeKey) const;

  const std::string name;
  const std::vector<uint16_t> portWidthIn;
  const std::vector<uint16_t> portWidthOut;
};

} // namespace eda::gate::translator

using LinkKey = eda::gate::translator::LinkKey;
using CellTypeKey = eda::gate::translator::CellTypeKey;

namespace std {

template<>
struct hash<LinkKey> {
  size_t operator()(const LinkKey &linkKey) const {
    size_t hash = std::hash<Operation*>()(linkKey.op);
    eda::util::hash_combine(hash, linkKey.portNum);
    eda::util::hash_combine(hash, linkKey.bitNum);
    return hash;
  }
};

template<>
struct hash<CellTypeKey> {
  size_t operator()(const CellTypeKey &cellTypeKey) const {
    size_t hash = std::hash<std::string>()(cellTypeKey.name);
    for (size_t i = 0; i < cellTypeKey.portWidthIn.size(); i++) {
      eda::util::hash_combine(hash, cellTypeKey.portWidthIn[i]);
    }
    for (size_t i = 0; i < cellTypeKey.portWidthOut.size(); i++) {
      eda::util::hash_combine(hash, cellTypeKey.portWidthOut[i]);
    }
    return hash;
  }
};

} // namespace std

namespace eda::gate::translator {

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
  static MLIRModule loadFromMLIR(const std::string &fileName,
                                 const std::string &string);
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
 * @brief The top level module which does translation from 'FIRRTL' to net.
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

std::vector<CellTypeID> getNet(const std::string &inputFilePath);

bool printNet(const std::string &inputFilePath,
              const std::string &outputDir);
bool printNet(const std::vector<CellTypeID> netlist,
              const std::string &outFileName);

} // namespace eda::gate::translator

std::unique_ptr<Pass> createCHIRRTLToLowFIRRTLPass();
std::unique_ptr<Pass> createLowFIRRTLToNetPass(
    std::shared_ptr<std::vector<CellTypeID>> resultNetlist);
