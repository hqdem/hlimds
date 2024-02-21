//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "fir_to_model2.h"

#include "gate/model2/celltype.h"
#include "util/assert.h"

#include "circt/Conversion/Passes.h"
#include "circt/Dialect/FIRRTL/CHIRRTLDialect.h"
#include "circt/Dialect/FIRRTL/FIRParser.h"
#include "circt/Dialect/FIRRTL/FIRRTLDialect.h"
#include "circt/Dialect/FIRRTL/FIRRTLOps.h"
#include "circt/Dialect/FIRRTL/FIRRTLTypes.h"
#include "circt/Dialect/FIRRTL/Passes.h"
#include "circt/Dialect/OM/OMDialect.h"
#include "circt/Dialect/SV/SVDialect.h"
#include "circt/Support/LLVM.h"
#include "circt/Support/Passes.h"

#include "llvm/ADT/APSInt.h"
#include "llvm/Support/SourceMgr.h"

#include "mlir/IR/Dialect.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Support/Timing.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/Passes.h"

#include <fstream>
#include <iostream>
#include <map>
#include <stack>
#include <unordered_set>
#include <vector>

using AddPrimOp = circt::firrtl::AddPrimOp;
using AndPrimOp = circt::firrtl::AndPrimOp;
using AndRPrimOp = circt::firrtl::AndRPrimOp;
using ArrayAttr = mlir::ArrayAttr;
template<typename Type>
using ArrayRef = mlir::ArrayRef<Type>;
using AsAsyncResetPrimOp = circt::firrtl::AsAsyncResetPrimOp;
using AsClockPrimOp = circt::firrtl::AsClockPrimOp;
using Attribute = mlir::Attribute;
using BitsPrimOp = circt::firrtl::BitsPrimOp;
using CHIRRTLDialect = circt::chirrtl::CHIRRTLDialect;
using CatPrimOp = circt::firrtl::CatPrimOp;
using CellID = eda::gate::model::CellID;
using CellProperties = eda::gate::model::CellProperties;
using CellSymbol = eda::gate::model::CellSymbol;
using CircuitOp = circt::firrtl::CircuitOp;
using ClockType = circt::firrtl::ClockType;
using CompanionMode = circt::firrtl::CompanionMode;
using ConnectOp = circt::firrtl::ConnectOp;
using ConstCastOp = circt::firrtl::ConstCastOp;
using ConstantOp = circt::firrtl::ConstantOp;
using ConventionAttr = circt::firrtl::ConventionAttr;
using DShlPrimOp = circt::firrtl::DShlPrimOp;
using DShlwPrimOp = circt::firrtl::DShlwPrimOp;
using DShrPrimOp = circt::firrtl::DShrPrimOp;
using DefaultTimingManager = mlir::DefaultTimingManager;
using DialectRegistry = mlir::DialectRegistry;
using Direction = circt::firrtl::Direction;
using DivPrimOp = circt::firrtl::DivPrimOp;
using EQPrimOp = circt::firrtl::EQPrimOp;
using FConnectLike = circt::firrtl::FConnectLike;
using FExtModuleOp = circt::firrtl::FExtModuleOp;
using FModuleLike = circt::firrtl::FModuleLike;
using FModuleOp = circt::firrtl::FModuleOp;
using FIRParserOptions = circt::firrtl::FIRParserOptions;
using FIRRTLBaseType = circt::firrtl::FIRRTLBaseType;
using FIRRTLDialect = circt::firrtl::FIRRTLDialect;
using GEQPrimOp = circt::firrtl::GEQPrimOp;
using GTPrimOp = circt::firrtl::GTPrimOp;
using HeadPrimOp = circt::firrtl::HeadPrimOp;
using InstanceOp = circt::firrtl::InstanceOp;
using IntegerType = circt::IntegerType;
using LEQPrimOp = circt::firrtl::LEQPrimOp;
using LLVMStringLiteral = llvm::StringLiteral;
using LLVMStringRef = llvm::StringRef;
using LTPrimOp = circt::firrtl::LTPrimOp;
using LinkEnd = eda::gate::model::LinkEnd;
using LogicalResult = mlir::LogicalResult;
using MLIRContext = mlir::MLIRContext;
using ModuleOp = mlir::ModuleOp;
using MulPrimOp = circt::firrtl::MulPrimOp;
using MuxPrimOp = circt::firrtl::MuxPrimOp;
using NEQPrimOp = circt::firrtl::NEQPrimOp;
using NameKindEnum = circt::firrtl::NameKindEnum;
using NetBuilder = eda::gate::model::NetBuilder;
using Net = eda::gate::model::Net;
using NegPrimOp = circt::firrtl::NegPrimOp;
using NotPrimOp = circt::firrtl::NotPrimOp;
using OMDialect = circt::om::OMDialect;
template<typename OperationType>
using OpConversionPattern = mlir::OpConversionPattern<OperationType>;
using Operation = mlir::Operation;
template<typename Type = void>
using OperationPass = mlir::OperationPass<Type>;
using OpPassManager = mlir::OpPassManager;
using OrPrimOp = circt::firrtl::OrPrimOp;
using OrRPrimOp = circt::firrtl::OrRPrimOp;
template<typename Type>
using OwningOpRef = mlir::OwningOpRef<Type>;
using Pass = mlir::Pass;
using PassManager = mlir::PassManager;
using PortInfo = circt::firrtl::PortInfo;
using PropAssignOp = circt::firrtl::PropAssignOp;
using PropertyType = circt::firrtl::PropertyType;
using RegOp = circt::firrtl::RegOp;
using RegResetOp = circt::firrtl::RegResetOp;
using Region = mlir::Region;
using RemPrimOp = circt::firrtl::RemPrimOp;
using ResetType = circt::firrtl::ResetType;
using SVDialect = circt::sv::SVDialect;
using ShlPrimOp = circt::firrtl::ShlPrimOp;
using ShrPrimOp = circt::firrtl::ShrPrimOp;
using SourceMgr = llvm::SourceMgr;
using StrictConnectOp = circt::firrtl::StrictConnectOp;
template<typename Type>
using StringAttr = mlir::StringAttr;
using StringRef = mlir::StringRef;
using SubPrimOp = circt::firrtl::SubPrimOp;
using TailPrimOp = circt::firrtl::TailPrimOp;
using TypeID = mlir::TypeID;
using Type = mlir::Type;
using Value = mlir::Value;
using WireOp = circt::firrtl::WireOp;
using XorPrimOp = circt::firrtl::XorPrimOp;
using XorRPrimOp = circt::firrtl::XorRPrimOp;

namespace PreserveAggregate = circt::firrtl::PreserveAggregate;
namespace PreserveValues = circt::firrtl::PreserveValues;
namespace chirrtl = circt::chirrtl;
namespace firrtl = circt::firrtl;
namespace hw = circt::hw;
namespace model = eda::gate::model;
namespace om = circt::om;
namespace sv = circt::sv;

namespace eda::gate::model {

MLIRModule MLIRModule::loadFromMLIR(const std::string &string) {
  auto context = std::make_unique<MLIRContext>();
  context->getOrLoadDialect<FIRRTLDialect>();
  auto moduleOp = mlir::parseSourceString<ModuleOp>(string, context.get());
  return { std::move(context), std::move(moduleOp) };
}

MLIRModule MLIRModule::loadFromMLIRFile(const std::string &filename) {
  std::ifstream file{ filename };
  std::stringstream buf;

  uassert(file, "File doesn't exist!");

  buf << file.rdbuf();
  
  return loadFromMLIR(buf.str());
}

MLIRModule MLIRModule::loadFromFIRFile(const std::string &filename) {
  DefaultTimingManager tm;
  auto ts = tm.getRootScope();
  SourceMgr sourceMgr;
  auto file = mlir::openInputFile(filename);

  uassert(file, "File doesn't exist!");

  sourceMgr.AddNewSourceBuffer(std::move(file), llvm::SMLoc());
  // sourceMgr.setIncludeDirs(includeDirs);
  auto context = std::make_unique<MLIRContext>();
  context->loadDialect<CHIRRTLDialect, FIRRTLDialect,
                       OMDialect, SVDialect>();

  auto parserTimer = ts.nest("FIR Parser");
  FIRParserOptions options;
  /// TODO: There could be some annotation files as well as '.omir' files.
  options.numAnnotationFiles = 0;

  auto moduleOp = importFIRFile(sourceMgr, context.get(), parserTimer, options);

  return { std::move(context), std::move(moduleOp) };
}

void MLIRModule::print(llvm::raw_ostream &os) { moduleOp->print(os); }

ModuleOp MLIRModule::getRoot() {
  return moduleOp.get();
}

MLIRModule::MLIRModule(MLIRModule &&oth)
    : context(std::move(oth.context)), moduleOp(std::move(oth.moduleOp)) {}

MLIRModule &MLIRModule::operator=(MLIRModule &&oth) {
  moduleOp = std::move(oth.moduleOp);
  context = std::move(oth.context);
  return *this;
}

MLIRModule MLIRModule::clone() { return { context, moduleOp->clone() }; }

MLIRContext *MLIRModule::getContext() { return moduleOp->getContext(); }

MLIRModule::MLIRModule(std::shared_ptr<MLIRContext> context,
                       mlir::OwningOpRef<ModuleOp> &&moduleOp)
    : context(std::move(context)), moduleOp(std::move(moduleOp)) {}

Translator::Translator(MLIRModule &&module)
    : module(std::move(module)),
      passManager(this->module.getContext()) {
  resultNetList = std::make_unique<std::vector<CellTypeID>>();
}


void Translator::addPass(std::unique_ptr<Pass> pass) {
  passManager.addPass(std::move(pass));
}

void Translator::runPasses() {
  ModuleOp moduleOp = module.getRoot();
  if (failed(passManager.run(moduleOp))) {
    std::cout << "Some passes failed!\n" << std::endl;
  }
}

void Translator::clearPasses() {
  passManager.clear();
}

void Translator::printFIRRTL() {
  std::string buf;
  llvm::raw_string_ostream os{buf};
  module.print(os);
  std::cout << buf << std::endl;
}

std::shared_ptr<std::vector<CellTypeID>> Translator::translate() {
  addPass(createCHIRRTLToLowFIRRTLPass());
  runPasses();
  clearPasses();
  printFIRRTL();
  addPass(createLowFIRRTLToModel2Pass(resultNetList));
  runPasses();
  clearPasses();
  return resultNetList;
}

bool CellKey::operator==(const CellKey &cellKey) const {
  return (operation == cellKey.operation &&
          portNumber == cellKey.portNumber &&
          bitNumber == cellKey.bitNumber);
}

} // namespace eda::gate::model

namespace {
bool isInstance(const std::string &operationName) {
  return (operationName == InstanceOp::getOperationName());
}

bool isReg(const std::string &operationName) {
  return (operationName == RegOp::getOperationName());
}

bool isRegReset(const std::string &operationName) {
  return (operationName == RegResetOp::getOperationName());
}

bool isAnyReg(const std::string &operationName) {
  return (isReg(operationName) || isRegReset(operationName));
}

bool isSynthesizable(const std::string &operationName) {
  return (operationName == EQPrimOp::getOperationName()    ||
          operationName == NEQPrimOp::getOperationName()   ||
          operationName == LTPrimOp::getOperationName()    ||
          operationName == LEQPrimOp::getOperationName()   ||
          operationName == GTPrimOp::getOperationName()    ||
          operationName == GEQPrimOp::getOperationName()   ||
          operationName == NegPrimOp::getOperationName()   ||
          operationName == AddPrimOp::getOperationName()   ||
          operationName == SubPrimOp::getOperationName()   ||
          operationName == MulPrimOp::getOperationName()   ||
          operationName == DivPrimOp::getOperationName()   ||
          operationName == RemPrimOp::getOperationName()   ||
          operationName == MuxPrimOp::getOperationName()   ||
          operationName == ShlPrimOp::getOperationName()   ||
          operationName == ShrPrimOp::getOperationName()   ||
          operationName == DShlPrimOp::getOperationName()  ||
          operationName == DShlwPrimOp::getOperationName() ||
          operationName == DShrPrimOp::getOperationName()  ||
          operationName == CatPrimOp::getOperationName()   ||
          operationName == HeadPrimOp::getOperationName()  ||
          operationName == TailPrimOp::getOperationName()  ||
          operationName == BitsPrimOp::getOperationName()  ||
          operationName == AndRPrimOp::getOperationName()  ||
          operationName == XorRPrimOp::getOperationName()  ||
          operationName == OrRPrimOp::getOperationName());
}

bool isTriviallySynthesizable(const std::string &operationName) {
  return (operationName == AndPrimOp::getOperationName()     ||
          operationName == OrPrimOp::getOperationName()      ||
          operationName == XorPrimOp::getOperationName()     ||
          operationName == NotPrimOp::getOperationName()     ||
          operationName == AsClockPrimOp::getOperationName() ||
          operationName == ConstCastOp::getOperationName()   ||
          operationName == AsAsyncResetPrimOp::getOperationName());
}

bool isStrictConnect(const std::string &operationName) {
  return (operationName == StrictConnectOp::getOperationName());
}

bool isWire(const std::string &operationName) {
  return (operationName == WireOp::getOperationName());
}

bool isOutput(const std::string &operationName) {
  return (operationName == "output");
}

bool isOmitted(const std::string &operationName) {
  return (operationName == PropAssignOp::getOperationName()    ||
          operationName == ConstantOp::getOperationName()      ||
          operationName == WireOp::getOperationName()          ||
          operationName == FModuleOp::getOperationName());
}

uint findOpResultNumber(Value value,
                        Operation *operation,
                        FModuleOp fModuleOp) {
  uint opResultNumber = 0;
  // If the source operation does not exist - it is the input.
  if (operation == nullptr) {
    uint inputCount = 0;
    for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
      if (fModuleOp.getPortDirection(i) == Direction::In) {
        auto &&inputValue = fModuleOp.getArgument(i);
        if (inputValue == value) {
          opResultNumber = inputCount;
          return opResultNumber;
        }
        inputCount++;
      }
    }
    for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
       if (fModuleOp.getPortDirection(i) == Direction::Out) {
        auto &&inputValue = fModuleOp.getArgument(i);
        if (inputValue == value) {
          opResultNumber = inputCount;
          return opResultNumber;
        }
        inputCount++;
      }
    }
  } else {
    // 'InstanceOp's are processed diffirently from other operations.
    std::string operationName = operation->getName().getIdentifier().str();
    if (isInstance(operationName)) {
      auto instanceOp = mlir::dyn_cast<InstanceOp>(operation);
      uint resultCount = 0;
      for (uint i = 0; i < instanceOp->getNumResults(); i++) {
        if (instanceOp.getPortDirection(i) == Direction::Out) {
          if (instanceOp->getResult(i) == value) {
            opResultNumber = resultCount;
            return opResultNumber;
          }
          resultCount++;
        }
      }
    } else {
      for (uint i = 0; i < operation->getNumResults(); i++) {
        if (operation->getResult(i) == value) {
          opResultNumber = i;
          return opResultNumber;
        }
      }
    }
  }

  uassert(false, "Operation result is not found!");

  return opResultNumber;
}

uint findOpOperandNumber(Value value,
                         Operation *operation,
                         FModuleOp fModuleOp) {
  uint opOperandNumber = 0;
  // If the source operation does not exist - it is the output.
  if (operation == nullptr) {
    uint outputCount = 0;
    for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
      if (fModuleOp.getPortDirection(i) == Direction::Out) {
        auto &&outputValue = fModuleOp.getArgument(i);
        if (outputValue == value) {
          opOperandNumber = outputCount;
          return opOperandNumber;
        }
        outputCount++;
      }
    }
  } else {
    // 'InstanceOp's are processed diffirently from other operations.
    std::string operationName = operation->getName().getIdentifier().str();
    if (isInstance(operationName)) {
      auto instanceOp = mlir::dyn_cast<InstanceOp>(operation);
      uint argumentCount = 0;
      for (uint i = 0; i < instanceOp->getNumResults(); i++) {
        if (instanceOp.getPortDirection(i) == Direction::In) {
          if (instanceOp->getResult(i) == value) {
            opOperandNumber = argumentCount;
            return opOperandNumber;
          }
          argumentCount++;
        }
      }
    } else if (isAnyReg(operationName)) {
      uint i = 0;
      if (operation->getResult(i) == value) {
        opOperandNumber = i;
        return opOperandNumber;
      }
      i++;
      for (; i < operation->getNumOperands(); i++) {
        if (operation->getOperand(i - 1) == value) {
          opOperandNumber = i;
          return opOperandNumber;
        }
      }
    } else if (isWire(operationName)) {
      if (operation->getResult(0) == value) {
        opOperandNumber = 0;
        return opOperandNumber;
      }
    } else {
      for (uint i = 0; i < operation->getNumOperands(); i++) {
        if (operation->getOperand(i) == value) {
          opOperandNumber = i;
          return opOperandNumber;
        }
      }
    }
  }

  uassert(false, "Operation operand is not found!");

  return opOperandNumber;
}

uint getTypeWidth(Type type) {
  auto &&firType = hw::type_cast<FIRRTLBaseType>(type);
  int typeWidthSentinel = firType.getBitWidthOrSentinel();

  uassert((typeWidthSentinel > 0), "Type width cannot be deduced!");

  return static_cast<uint>(typeWidthSentinel);
}

uint getFanoutCount(Operation *operation, const std::string &operationName) {
  uint fanoutCount = 0;
  if (isInstance(operationName)) {
    auto instanceOp = mlir::dyn_cast<InstanceOp>(operation);
    for (uint i = 0; i < instanceOp->getNumResults(); i++) {
      if (instanceOp.getPortDirection(i) == Direction::Out) {
        uint outputWidth = getTypeWidth(instanceOp.getResult(i).getType());
        fanoutCount += outputWidth;
      }
    }
  } else if (isAnyReg(operationName)) {
      for (uint i = 0; i < operation->getNumResults(); i++) {
        uint outputWidth = getTypeWidth(operation->getResult(i).getType());
        fanoutCount += outputWidth;
    }
  } else if (isOutput(operationName)) {
    fanoutCount = 0;
  } else {
    for (auto &&resultType : operation->getResultTypes()) {
      uint outputWidth = getTypeWidth(resultType);
      fanoutCount += outputWidth;
    }
  }
  return fanoutCount;
}

uint getFaninCount(Operation *operation, const std::string &operationName) {
  uint faninCount = 0;
  if (isInstance(operationName)) {
    auto instanceOp = mlir::dyn_cast<InstanceOp>(operation);
    for (uint i = 0; i < instanceOp->getNumResults(); i++) {
      if (instanceOp.getPortDirection(i) == Direction::In) {
        uint inputWidth = getTypeWidth(instanceOp.getResult(i).getType());
        faninCount += inputWidth;
      }
    }
  } else if (isReg(operationName) || isWire(operationName)) {
    faninCount += getTypeWidth(operation->getResult(0).getType());
    // Register must have a clock input.
    if (isReg(operationName)) {
      faninCount += 1;
    }
  } else if (isRegReset(operationName)) {
      // RegReset has a reset value of arbitrary width.
      for (uint i = 0; i < operation->getNumOperands(); i++) {
        uint inputWidth = getTypeWidth(operation->getOperand(i).getType());
        faninCount += inputWidth;
      }
      for (uint i = 0; i < operation->getNumResults(); i++) {
        uint inputWidth = getTypeWidth(operation->getResult(i).getType());
        faninCount += inputWidth;
      }
  } else if (isOutput(operationName)) {
    faninCount = 1;
  } else {
    for (auto &&operandType : operation->getOperandTypes()) {
      uint inputWidth = getTypeWidth(operandType);
      faninCount += inputWidth;
    }
  }
  return faninCount;
}

uint getInputCount(Operation *operation, const std::string &operationName) {
  uint inputCount = 0;
  if (isInstance(operationName)) {
    auto instanceOp = mlir::dyn_cast<InstanceOp>(operation);
    for (uint i = 0; i < instanceOp->getNumResults(); i++) {
      if (instanceOp.getPortDirection(i) == Direction::In) {
        inputCount++;
      }
    }
  } else if (isReg(operationName)) {
    inputCount = 2;
  } else if (isRegReset(operationName)) {
    inputCount = 4;
  } else if (isWire(operationName) ||
             isOutput(operationName)) {
    inputCount = 1;
  } else {
    inputCount = operation->getNumOperands();
  }
  return inputCount;
}

uint getOutputCount(Operation *operation, const std::string &operationName) {
  uint outputCount = 0;
  if (isInstance(operationName)) {
    auto instanceOp = mlir::dyn_cast<InstanceOp>(operation);
    for (uint i = 0; i < instanceOp->getNumResults(); i++) {
      if (instanceOp.getPortDirection(i) == Direction::Out) {
        outputCount++;
      }
    }
  } else if (isAnyReg(operationName) ||
             isWire(operationName)) {
    outputCount = 1;
  } else if (isOutput(operationName)) {
    outputCount = 0;
  } else {
    outputCount = operation->getNumResults();
  }
  return outputCount;
}

Operation *getSourceOperation(Operation *destOp, std::string &destOpName,
                              Value operand, uint inputNumber,
                              uint inputCount) {
  Operation *srcOp = nullptr;
  if (isInstance(destOpName) ||
      isWire(destOpName)) {
    for (auto *user : operand.getUsers()) {
      if (auto connect = mlir::dyn_cast<FConnectLike>(user)) {
        if (connect.getDest() != operand)
          continue;
        srcOp = connect.getSrc().getDefiningOp();
      }
    }
  } else {
    srcOp = operand.getDefiningOp();
  }
  return srcOp;
}

Value getDestValue(Operation *destOp, std::string &destOpName,
                   uint inputNumber, uint inputCount) {
  Value operand;
  if (isInstance(destOpName)) {
    operand = destOp->getResult(inputNumber);
  } else if (isWire(destOpName)) {
    operand = destOp->getResult(0);
  } else {
    operand = destOp->getOperand(inputNumber);
  }
  return operand;
}

std::vector<LinkEnd> getLinkEnds(Operation *destOp, FModuleOp fModuleOp,
    std::unordered_map<CellKey, CellID> &cellKeyToCellIDOuts) {
  std::vector<LinkEnd> linkEnds;
  std::string destOpName = destOp->getName().getIdentifier().str();
  uint operandCount = destOp->getNumOperands();
  for (uint i = 0; i < operandCount; i++) {
    Value operand = getDestValue(destOp, destOpName, i, operandCount);
    Operation *srcOp = getSourceOperation(destOp, destOpName, operand,
                                          i, operandCount);
    uint resNumber = findOpResultNumber(operand, srcOp, fModuleOp);
    uint inWidth = getTypeWidth(operand.getType());

    for (uint j = 0; j < inWidth; j++) {
      CellKey srcKey(srcOp, resNumber, j);
      CellID cellSrcID = cellKeyToCellIDOuts[srcKey];

      uassert((cellSrcID != model::OBJ_NULL_ID),
              "No CellIDs for a CellKey have been found!");

      linkEnds.push_back(LinkEnd(cellSrcID));
    }
  }
  return linkEnds;
}

void generateInputs(FModuleOp fModuleOp,
                    NetBuilder *netBuilder,
                    std::unordered_map<CellKey, CellID> &cellKeyToCellIDOuts) {
  // Inputs.
  uint inNumber = 0;
  for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
    if (fModuleOp.getPortDirection(i) == Direction::In &&
        !(mlir::dyn_cast<PropertyType>(fModuleOp.getPortType(i)))) {
      uint portWidth = getTypeWidth(fModuleOp.getPortType(i));
      for (uint j = 0; j < portWidth; j++) {
        CellID cellID = makeCell(CellSymbol::IN);
        CellKey cellKey(nullptr, inNumber, j);
        cellKeyToCellIDOuts.emplace(std::make_pair(cellKey, cellID));
        netBuilder->addCell(cellID);
      }
      inNumber++;
    }
  }
  // Constants.
  CellID cellIDForOne = model::OBJ_NULL_ID;
  CellID cellIDForZero = model::OBJ_NULL_ID;
  fModuleOp.walk([&](ConstantOp constantOp) {
    uint outputWidth = getTypeWidth(constantOp.getResult().getType());
    auto &&value = constantOp.getValue();
    for (size_t i = 0; i < outputWidth; i++) {
      uint extractedBit = value.extractBitsAsZExtValue(1, i);
      CellID cellID = model::OBJ_NULL_ID;
      if (extractedBit == 1) {
        if (cellIDForOne == model::OBJ_NULL_ID) {
          cellIDForOne = makeCell(CellSymbol::ONE);
        }
        cellID = cellIDForOne;
      } else {
        if (cellIDForZero == model::OBJ_NULL_ID) {
          cellIDForZero = makeCell(CellSymbol::ZERO);
        }
        cellID = cellIDForZero;
      }
      CellKey cellKey(constantOp, 0, i);
      cellKeyToCellIDOuts.emplace(std::make_pair(cellKey, cellID));
      netBuilder->addCell(cellID);
    }
  });
}

void generateOutputs(FModuleOp fModuleOp,
                     NetBuilder *netBuilder,
                     std::unordered_map<CellKey, CellID> &cellKeyToCellIDIns,
                     std::unordered_map<CellKey, CellID> &cellKeyToCellIDOuts) {
  size_t outNumber = 0;
  size_t inCount = 0;
  for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
    if (fModuleOp.getPortDirection(i) == Direction::In &&
        !(mlir::dyn_cast<PropertyType>(fModuleOp.getPortType(i)))) {
      inCount++;
    }
  }
  for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
    if (fModuleOp.getPortDirection(i) == Direction::Out &&
        !(mlir::dyn_cast<PropertyType>(fModuleOp.getPortType(i)))) {
      uint portWidth = getTypeWidth(fModuleOp.getPortType(i));

      for (uint j = 0; j < portWidth; j++) {
        CellID cellBufID = makeCell(CellSymbol::BUF,
                                    LinkEnd(model::OBJ_NULL_ID));
        netBuilder->addCell(cellBufID);
        CellID cellOutID = makeCell(CellSymbol::OUT, LinkEnd(cellBufID));
        netBuilder->addCell(cellOutID);
        CellKey cellKeyIn(nullptr, outNumber, j);
        CellKey cellKeyOut(nullptr, outNumber + inCount, j);
        cellKeyToCellIDIns.emplace(std::make_pair(cellKeyIn, cellBufID));
        cellKeyToCellIDOuts.emplace(std::make_pair(cellKeyOut, cellBufID));
      }
      outNumber++;
    }
  }
}

void generateWires(FModuleOp fModuleOp,
                   NetBuilder *netBuilder,
                   std::unordered_map<CellKey, CellID> &cellKeyToCellIDIns,
                   std::unordered_map<CellKey, CellID> &cellKeyToCellIDOuts) {
  fModuleOp.walk([&](WireOp wireOp) {
    uint outputWidth = getTypeWidth(wireOp.getResult().getType());
    for (size_t i = 0; i < outputWidth; i++) {
      CellID cellID = makeCell(CellSymbol::BUF, LinkEnd(model::OBJ_NULL_ID));
      netBuilder->addCell(cellID);
      CellKey cellKeyIn(wireOp, 0, i);
      CellKey cellKeyOut(wireOp, 0, i);
      cellKeyToCellIDIns.emplace(std::make_pair(cellKeyIn, cellID));
      cellKeyToCellIDOuts.emplace(std::make_pair(cellKeyOut, cellID));
      // At the beginning wires exist without any connections.
    }
  });
}

CellSymbol getCellSymbol(Operation *operation) {
  CellSymbol cellSymbol;
  if (operation != nullptr) {
    std::string operationName =
        operation->getName().getIdentifier().str();
    if (operationName == ConstCastOp::getOperationName()   ||
        operationName == AsClockPrimOp::getOperationName() ||
        operationName == AsAsyncResetPrimOp::getOperationName()) {
      cellSymbol = CellSymbol::BUF;
    } else if (operationName == InstanceOp::getOperationName()) {
      cellSymbol = CellSymbol::HARD;
    } else if (isSynthesizable(operationName)) {
      cellSymbol = CellSymbol::SOFT;
    } else if (operationName == AndPrimOp::getOperationName()) {
      cellSymbol = CellSymbol::AND;
    } else if (operationName == OrPrimOp::getOperationName()) {
      cellSymbol = CellSymbol::OR;
    } else if (operationName == XorPrimOp::getOperationName()) {
      cellSymbol = CellSymbol::XOR;
    } else if (operationName == NotPrimOp::getOperationName()) {
      cellSymbol = CellSymbol::NOT;
    } else if (operationName == RegOp::getOperationName()) {
      cellSymbol = CellSymbol::DFF;
    } else if (operationName == RegResetOp::getOperationName()) {
      cellSymbol = CellSymbol::DFFrs;
    } else {
      cellSymbol = CellSymbol::HARD;
    }
  } else {
    cellSymbol = CellSymbol::OUT;
  }
  return cellSymbol;
}

uint getModel2InPortNum(Operation *operation, uint portNumber, uint bitNumber) {
  uint model2InPortNum = 0;
  if (operation == nullptr) {
    return model2InPortNum;
  } else {
    // 'InstanceOp's are processed diffirently from other operations.
    std::string operationName = operation->getName().getIdentifier().str();
    if (isInstance(operationName)) {
      auto instanceOp = mlir::dyn_cast<InstanceOp>(operation);
      for (uint i = 0; i < portNumber; i++) {
        if (instanceOp.getPortDirection(i) == Direction::In) {
          model2InPortNum += getTypeWidth(instanceOp->getResult(i).getType());
        }
      }
      model2InPortNum += bitNumber;
    } else if (isTriviallySynthesizable(operationName) ||
               isWire(operationName)) {
      for (; model2InPortNum < portNumber; model2InPortNum++);
    } else if (isAnyReg(operationName)) {
      for (; model2InPortNum < portNumber; model2InPortNum++);
    } else {
      for (uint i = 0; i < portNumber; i++) {
          model2InPortNum += getTypeWidth(operation->getOperand(i).getType());
      }
      model2InPortNum += bitNumber;
    }
  }
  return model2InPortNum;
}

uint getModel2OutPortNum(Operation *operation,
                         uint portNumber,
                         uint bitNumber) {
  uint model2OutPortNum = 0;
  if (operation == nullptr) {
    return model2OutPortNum;
  } else {
    // 'InstanceOp's are processed diffirently from other operations.
    std::string operationName = operation->getName().getIdentifier().str();
    if (isInstance(operationName)) {
      auto instanceOp = mlir::dyn_cast<InstanceOp>(operation);
      for (uint i = 0; i < portNumber; i++) {
        if (instanceOp.getPortDirection(i) == Direction::Out) {
          model2OutPortNum += getTypeWidth(instanceOp->getResult(i).getType());
        }
      }
      model2OutPortNum += bitNumber;
    } else if (isTriviallySynthesizable(operationName) ||
               isWire(operationName)) {
      for (; model2OutPortNum < portNumber; model2OutPortNum++);
    } else if (isAnyReg(operationName)) {
      for (; model2OutPortNum < portNumber; model2OutPortNum++);
    } else {
      for (uint i = 0; i < portNumber; i++) {
        model2OutPortNum += getTypeWidth(operation->getResult(i).getType());
      }
      model2OutPortNum += bitNumber;
    }
  }
  return model2OutPortNum;
}

void processOperation(Operation *destOp, std::string &destOpName,
    FModuleOp fModuleOp, NetBuilder *netBuilder,
    std::unordered_map<CellKey, CellID> &cellKeyToCellIDIns,
    std::unordered_map<CellKey, CellID> &cellKeyToCellIDOuts) {
  CellSymbol cellSymbol = getCellSymbol(destOp);
  uint inputCount = getInputCount(destOp, destOpName);
  uint faninCount = getFaninCount(destOp, destOpName);
  if (!isOmitted(destOpName)) {
    if (isInstance(destOpName)) {
      uint fanoutCount = getFanoutCount(destOp, destOpName);
      auto instanceOp = mlir::dyn_cast<InstanceOp>(destOp);
      const auto &cellTypeName = instanceOp.getModuleName().str();
      CellTypeID cellTypeID = makeCellType(cellTypeName,
                                           cellSymbol,
                                           CellProperties(false, false,
                                                          false, false,
                                                          false, false,
                                                          false),
                                           faninCount,
                                           fanoutCount);
      std::vector<LinkEnd> linkEnds;
      for (uint i = 0; i < faninCount; i++) {
        linkEnds.push_back(LinkEnd(model::OBJ_NULL_ID));
      }
      CellID cellDestID = makeCell(cellTypeID, linkEnds);
      netBuilder->addCell(cellDestID);
      uint outputCount = getOutputCount(destOp, destOpName);
      for (uint i = 0; i < outputCount; i++) {
        auto &&result = destOp->getResult(i + inputCount);
        uint outWidth = getTypeWidth(result.getType());
        for (uint j = 0; j < outWidth; j++) {
          CellKey outKey(destOp, i, j);
          cellKeyToCellIDOuts.emplace(std::make_pair(outKey, cellDestID));
        }
      }
      for (uint i = 0; i < inputCount; i++) {
        auto &&result = destOp->getResult(i);
        uint inWidth = getTypeWidth(result.getType());
        for (uint j = 0; j < inWidth; j++) {
          CellKey inKey(destOp, i, j);
          cellKeyToCellIDIns.emplace(std::make_pair(inKey, cellDestID));
        }
      }
    } else if (isSynthesizable(destOpName)) {
      uint fanoutCount = getFanoutCount(destOp, destOpName);
      auto linkEnds = getLinkEnds(destOp, fModuleOp, cellKeyToCellIDOuts);
      const auto &cellTypeName = destOp->getName().stripDialect().str();
      CellTypeID cellTypeID = makeCellType(cellTypeName,
                                           cellSymbol,
                                           CellProperties(false, false,
                                                          false, false,
                                                          false, false,
                                                          false),
                                           faninCount,
                                           fanoutCount);
      CellID cellDestID = makeCell(cellTypeID, linkEnds);
      netBuilder->addCell(cellDestID);
      uint outputCount = getOutputCount(destOp, destOpName);
      for (uint i = 0; i < outputCount; i++) {
        auto &&result = destOp->getResult(i);
        uint outWidth = getTypeWidth(result.getType());
        for (uint j = 0; j < outWidth; j++) {
          CellKey destKey(destOp, i, j);
          cellKeyToCellIDOuts.emplace(std::make_pair(destKey, cellDestID));
        }
      }
    } else if (isTriviallySynthesizable(destOpName)) {
      auto linkEnds = getLinkEnds(destOp, fModuleOp, cellKeyToCellIDOuts);
      CellTypeID cellTypeID = getCellTypeID(cellSymbol);
      uint dataWidth = getTypeWidth(destOp->getResult(0).getType());
      for (uint j = 0; j < dataWidth; j++) {
        std::vector<LinkEnd> linkEndsForOne;
        for (uint i = 0; i < inputCount; i++) {
          linkEndsForOne.push_back(linkEnds[i * dataWidth + j]);
        }
        CellID cellDestID = makeCell(cellTypeID, linkEndsForOne);
        netBuilder->addCell(cellDestID);
        CellKey destKey(destOp, 0, j);
        cellKeyToCellIDOuts.emplace(std::make_pair(destKey, cellDestID));
      }
    } else if (isReg(destOpName)) {
      auto linkEnds = getLinkEnds(destOp, fModuleOp, cellKeyToCellIDOuts);
      CellTypeID cellTypeID = getCellTypeID(cellSymbol);
      uint dataWidth = faninCount - 1;
      for (uint j = 0; j < dataWidth; j++) {
        // DFF(q, d, clk).
        std::vector<LinkEnd> linkEndsForOne;
        linkEndsForOne.push_back(LinkEnd(model::OBJ_NULL_ID));
        linkEndsForOne.push_back(linkEnds.front());
        CellID cellDestID = makeCell(cellTypeID, linkEndsForOne);
        netBuilder->addCell(cellDestID);
        CellKey outKey(destOp, 0, j);
        cellKeyToCellIDOuts.emplace(std::make_pair(outKey, cellDestID));
        CellKey inKey(destOp, 0, j);
        cellKeyToCellIDIns.emplace(std::make_pair(inKey, cellDestID));
      }
    } else if (isRegReset(destOpName)) {
      auto linkEnds = getLinkEnds(destOp, fModuleOp, cellKeyToCellIDOuts);
      CellTypeID cellTypeID = getCellTypeID(cellSymbol);
      uint dataWidth = getTypeWidth(destOp->getResult(0).getType());
      uint resetValueWidth = getTypeWidth(destOp->getOperand(2).getType());
      for (uint j = 0, k = 0; j < dataWidth; j++) {
        // DFFrs(q, d, clk, rst, set).
        std::vector<LinkEnd> linkEndsForOne;
        linkEndsForOne.push_back(LinkEnd(model::OBJ_NULL_ID));
        linkEndsForOne.push_back(linkEnds.front());
        CellID negMidID = makeCell(CellSymbol::NOT, linkEnds[k + 2]);
        netBuilder->addCell(negMidID);
        CellID andMidRID = makeCell(CellSymbol::AND,
                                    LinkEnd(negMidID),
                                    linkEnds[1]);
        linkEndsForOne.push_back(LinkEnd(andMidRID));
        netBuilder->addCell(andMidRID);
        CellID andMidSID = makeCell(CellSymbol::AND,
                                    linkEnds[k + 2],
                                    linkEnds[1]);
        linkEndsForOne.push_back(LinkEnd(andMidSID));
        netBuilder->addCell(andMidSID);
        CellID cellDestID = makeCell(cellTypeID, linkEndsForOne);
        netBuilder->addCell(cellDestID);
        CellKey outKey(destOp, 0, j);
        cellKeyToCellIDOuts.emplace(std::make_pair(outKey, cellDestID));
        CellKey inKey(destOp, 0, j);
        cellKeyToCellIDIns.emplace(std::make_pair(inKey, cellDestID));
        // If a reset value doesn't have got a width of one - it width must be
        // equal to the width of the data.
        if (resetValueWidth != 1) {
          k++;
        }
      }
    } else if (isStrictConnect(destOpName)) {
      auto strictConnectOp = mlir::dyn_cast<StrictConnectOp>(destOp);
      Value fromValue = strictConnectOp.getSrc();
      Value toValue = strictConnectOp.getDest();
      Operation *fromOp = fromValue.getDefiningOp();
      Operation *toOp = toValue.getDefiningOp();
      Type type = fromValue.getType();
      uint typeWidth = getTypeWidth(type);
      uint outPortNum = findOpResultNumber(fromValue, fromOp, fModuleOp);
      uint inPortNum = findOpOperandNumber(toValue, toOp, fModuleOp);
      for (uint i = 0; i < typeWidth; i++) {
        CellKey srcKey(fromOp, outPortNum, i);
        CellID srcCellID = cellKeyToCellIDOuts[srcKey];
        CellKey destKey(toOp, inPortNum, i);
        CellID destCellID = cellKeyToCellIDIns[destKey];
        uint model2InPortNum = getModel2InPortNum(toOp, inPortNum, i);
        uint model2OutPortNum = getModel2OutPortNum(fromOp, outPortNum, i);
        netBuilder->connect(destCellID, model2InPortNum,
            LinkEnd(srcCellID, model2OutPortNum));
      }
    } else {
      uassert(false, "Unknown operation: " + destOpName + "! Abort.");
    }
  }
}
 
// Top-level operation.
LogicalResult generateModel(ModuleOp moduleOp,
    std::shared_ptr<std::vector<CellTypeID>> resultNetList) {
  CircuitOp circuitOp =
      *(moduleOp.getRegion().begin()->getOps<CircuitOp>().begin());
  for (auto &&fModuleOp : circuitOp.getBodyBlock()->getOps<FModuleOp>()) {
    NetBuilder netBuilder;
    std::unordered_map<CellKey, CellID> cellKeyToCellIDOuts;
    std::unordered_map<CellKey, CellID> cellKeyToCellIDIns;
    generateInputs(fModuleOp, &netBuilder, cellKeyToCellIDOuts);
    generateOutputs(fModuleOp, &netBuilder, cellKeyToCellIDIns,
                    cellKeyToCellIDOuts);
    generateWires(fModuleOp, &netBuilder, cellKeyToCellIDIns,
                  cellKeyToCellIDOuts);
    fModuleOp.walk([&](Operation *destOp) {
      std::string destOpName = destOp->getName().getIdentifier().str();
      processOperation(destOp, destOpName, fModuleOp, &netBuilder,
                       cellKeyToCellIDIns, cellKeyToCellIDOuts);
    });
    NetID netID = netBuilder.make();
    const std::string &cellName = fModuleOp->getName().getIdentifier().str();
    CellTypeID cellTypeID = makeCellType(cellName,
                                         netID,
                                         model::OBJ_NULL_ID,
                                         CellSymbol::SOFT,
                                         CellProperties(false, false, false,
                                                        false, false, false,
                                                        false),
                                         Net::get(netID).getInNum(),
                                         Net::get(netID).getOutNum());
    resultNetList->push_back(cellTypeID);
  }
  return mlir::success();
}

template <typename DerivedT>
class TranslatePass : public OperationPass<ModuleOp> {
public:
  using Base = TranslatePass;

  TranslatePass()
      : OperationPass<ModuleOp>(TypeID::get<DerivedT>()) {}
  TranslatePass(const TranslatePass &other)
      : OperationPass<ModuleOp>(other) {}

  /// Returns the command-line argument attached to this pass.
  static constexpr LLVMStringLiteral getArgumentName() {
    return LLVMStringLiteral("translate");
  }
  LLVMStringRef getArgument() const override { return "translate"; }

  LLVMStringRef getDescription() const override {
    return "Translate operations";
  }

  /// Returns the derived pass name.
  static constexpr LLVMStringLiteral getPassName() {
    return LLVMStringLiteral("Translator");
  }
  LLVMStringRef getName() const override { return "Translator"; }

  /// Support isa/dyn_cast functionality for the derived pass class.
  static bool classof(const Pass *pass) {
    return pass->getTypeID() == TypeID::get<DerivedT>();
  }

  /// A clone method to create a copy of this pass.
  std::unique_ptr<Pass> clonePass() const override {
    return std::make_unique<DerivedT>(*static_cast<const DerivedT *>(this));
  }

  /// Return the dialect that must be loaded in the context before this pass.
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<CHIRRTLDialect>();
    registry.insert<FIRRTLDialect>();
    registry.insert<OMDialect>();
    registry.insert<SVDialect>();
  }
};

enum class RandomKind { None, Mem, Reg, All };

/// TODO: Investigate what each of these options really mean (default for now).
struct FIRRTLLoweringOptions {
  bool disableOptimization = false;
  bool disableHoistingHWPassthrough = true;
  RandomKind disableRandom = RandomKind::None;

  PreserveValues::PreserveMode preserveMode =
      PreserveValues::PreserveMode::None;

  bool replSeqMem = false;
  std::string replSeqMemFile = "";
  bool ignoreReadEnableMem = true;

  bool exportChiselInterface = false;
  std::string chiselInterfaceOutDirectory = "";

  bool dedup = true;

  bool vbToBV = true;

  bool lowerMemories = true;

  PreserveAggregate::PreserveMode preserveAggregate =
      PreserveAggregate::PreserveMode::None;

  std::string blackBoxRootPath = "";

  CompanionMode companionMode = CompanionMode::Bind;

  /// TODO: The default value in 'Firtool.cpp' is 'true'.
  bool emitOMIR = false;
  std::string omirOutFile = "";

  bool disableAggressiveMergeConnections = false;

  bool isRandomEnabled(RandomKind kind) const {
    return disableRandom != RandomKind::All && disableRandom != kind;
  }
};

class CHIRRTLToLowFIRRTLPass : public TranslatePass<CHIRRTLToLowFIRRTLPass> {
public:
  void runOnOperation() override {
    ModuleOp moduleOp = getOperation();
    PassManager pm(moduleOp.getContext());
    // Default options for now.
    FIRRTLLoweringOptions opt;
    pm.nest<CircuitOp>().addPass(firrtl::createLowerIntrinsicsPass());

    pm.nest<CircuitOp>().addPass(firrtl::createInjectDUTHierarchyPass());

    pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
        firrtl::createDropNamesPass(opt.preserveMode));

    if (!opt.disableOptimization)
      pm.nest<CircuitOp>().nest<FModuleOp>().addPass(mlir::createCSEPass());

    pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
        firrtl::createLowerCHIRRTLPass());

    // Run LowerMatches before InferWidths, as the latter does not support the
    // match statement, but it does support what they lower to.
    pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
        firrtl::createLowerMatchesPass());

    // Width inference creates canonicalization opportunities.
    pm.nest<CircuitOp>().addPass(firrtl::createInferWidthsPass());

    pm.nest<CircuitOp>().addPass(
        firrtl::createMemToRegOfVecPass(opt.replSeqMem,
                                        opt.ignoreReadEnableMem));

    pm.nest<CircuitOp>().addPass(firrtl::createInferResetsPass());

    if (opt.exportChiselInterface) {
      if (opt.chiselInterfaceOutDirectory.empty()) {
        pm.nest<CircuitOp>().addPass(circt::createExportChiselInterfacePass());
      } else {
        pm.nest<CircuitOp>().addPass(
            circt::createExportSplitChiselInterfacePass(
                opt.chiselInterfaceOutDirectory));
      }
    }

    pm.nest<CircuitOp>().nestAny().addPass(firrtl::createDropConstPass());

    pm.nest<CircuitOp>().addPass(firrtl::createHoistPassthroughPass(
        !opt.disableOptimization &&
        !opt.disableHoistingHWPassthrough));
    pm.nest<CircuitOp>().addPass(firrtl::createProbeDCEPass());

    if (opt.dedup) {
      pm.nest<CircuitOp>().addPass(firrtl::createDedupPass());
    }

    pm.nest<CircuitOp>().addPass(firrtl::createWireDFTPass());

    if (opt.vbToBV) {
      pm.addNestedPass<CircuitOp>(firrtl::createLowerFIRRTLTypesPass(
          firrtl::PreserveAggregate::All, firrtl::PreserveAggregate::All));
      pm.addNestedPass<CircuitOp>(firrtl::createVBToBVPass());
    }

    if (!opt.lowerMemories) {
      pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
          firrtl::createFlattenMemoryPass());
    }
    // The input mlir file could be firrtl dialect so we might need to clean
    // things up.
    pm.addNestedPass<CircuitOp>(firrtl::createLowerFIRRTLTypesPass(
        opt.preserveAggregate, firrtl::PreserveAggregate::None));
    pm.nest<CircuitOp>().nestAny().addPass(firrtl::createExpandWhensPass());
    // Only enable expand whens if lower types is also enabled.
    auto &modulePM = pm.nest<CircuitOp>().nest<FModuleOp>();
    modulePM.addPass(firrtl::createSFCCompatPass());
    modulePM.addPass(firrtl::createLayerMergePass());
    modulePM.addPass(firrtl::createLayerSinkPass());

    pm.nest<CircuitOp>().addPass(firrtl::createLowerLayersPass());

    pm.nest<CircuitOp>().addPass(firrtl::createInlinerPass());

    // Preset the random initialization parameters for each module. The current
    // implementation assumes it can run at a time where every register is
    // currently in the final module it will be emitted in, all registers have
    // been created, and no registers have yet been removed.
    if (opt.isRandomEnabled(RandomKind::Reg))
      pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
          firrtl::createRandomizeRegisterInitPass());

    pm.nest<CircuitOp>().addPass(firrtl::createCheckCombLoopsPass());

    // If we parsed a FIRRTL file and have optimizations enabled, clean it up.
    if (!opt.disableOptimization) {
      pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
          circt::createSimpleCanonicalizerPass());
    }

    // Run the infer-rw pass, which merges read and write ports of a memory with
    // mutually exclusive enables.
    if (!opt.disableOptimization) {
      pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
          firrtl::createInferReadWritePass());
    }

    if (opt.replSeqMem) {
      pm.nest<CircuitOp>().addPass(firrtl::createLowerMemoryPass());
    }

    pm.nest<CircuitOp>().addPass(firrtl::createPrefixModulesPass());

    if (!opt.disableOptimization) {
      pm.nest<CircuitOp>().addPass(firrtl::createIMConstPropPass());

      pm.nest<CircuitOp>().addPass(firrtl::createHoistPassthroughPass(
        !opt.disableOptimization &&
        !opt.disableHoistingHWPassthrough));
      // Cleanup after hoisting passthroughs, for separation-of-concerns.
      pm.addPass(firrtl::createIMDeadCodeElimPass());
    }

    pm.addNestedPass<CircuitOp>(firrtl::createAddSeqMemPortsPass());

    pm.addPass(firrtl::createCreateSiFiveMetadataPass(
        opt.replSeqMem,
        opt.replSeqMemFile));

    pm.addNestedPass<CircuitOp>(firrtl::createExtractInstancesPass());
    // Run passes to resolve Grand Central features.  This should run before
    // BlackBoxReader because Grand Central needs to inform BlackBoxReader where
    // certain black boxes should be placed.  Note: all Grand Central Taps
    // related collateral is resolved entirely by LowerAnnotations.
    pm.addNestedPass<CircuitOp>(
        firrtl::createGrandCentralPass(opt.companionMode));

    // Read black box source files into the IR.
    StringRef blackBoxRoot = opt.blackBoxRootPath.empty()
                                ? StringRef()
                                : opt.blackBoxRootPath;
    pm.nest<CircuitOp>().addPass(
        firrtl::createBlackBoxReaderPass(blackBoxRoot));

    // Run SymbolDCE as late as possible, but before InnerSymbolDCE. This is for
    // hierpathop's and just for general cleanup.
    pm.addNestedPass<CircuitOp>(mlir::createSymbolDCEPass());

    // Run InnerSymbolDCE as late as possible, but before IMDCE.
    pm.addPass(firrtl::createInnerSymbolDCEPass());

    // The above passes, IMConstProp in particular, introduce additional
    // canonicalization opportunities that we should pick up here before we
    // proceed to output-specific pipelines.
    if (!opt.disableOptimization) {
      pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
          circt::createSimpleCanonicalizerPass());
      pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
          firrtl::createRegisterOptimizerPass());
      // Re-run IMConstProp to propagate constants produced by register
      // optimizations.
      pm.nest<CircuitOp>().addPass(firrtl::createIMConstPropPass());
      pm.addPass(firrtl::createIMDeadCodeElimPass());
    }

    if (opt.emitOMIR)
      pm.nest<CircuitOp>().addPass(
          firrtl::createEmitOMIRPass(opt.omirOutFile));

    // Always run this, required for legalization.
    pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
        firrtl::createMergeConnectionsPass(
            !opt.disableAggressiveMergeConnections));

    if (!opt.disableOptimization) {
      pm.nest<CircuitOp>().nest<FModuleOp>().addPass(
          firrtl::createVectorizationPass());
    }
    
    if (failed(runPipeline(pm, moduleOp))) {
      return signalPassFailure();
    }
  }
};

class LowFIRRTLToModel2Pass : public TranslatePass<LowFIRRTLToModel2Pass> {
private:
  std::shared_ptr<std::vector<CellTypeID>> resultNetList;
public:
  LowFIRRTLToModel2Pass(
      std::shared_ptr<std::vector<CellTypeID>> resultNetList) {
    this->resultNetList = resultNetList;
  }
  void runOnOperation() override {
    ModuleOp moduleOp = getOperation();
    if (failed(generateModel(moduleOp, resultNetList))) {
      signalPassFailure();
    }
  }
};

} // namespace

std::unique_ptr<Pass> createCHIRRTLToLowFIRRTLPass() {
  return std::make_unique<CHIRRTLToLowFIRRTLPass>();
}

std::unique_ptr<Pass> createLowFIRRTLToModel2Pass(
    std::shared_ptr<std::vector<CellTypeID>> resultNetList) {
  return std::make_unique<LowFIRRTLToModel2Pass>(resultNetList);
}