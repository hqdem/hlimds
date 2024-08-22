//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "firrtl_net.h"
#include "firrtl_net_utils.h"

#include "gate/model/celltype.h"
#include "gate/model/object.h"
#include "gate/model/printer/net_printer.h"
#include "util/assert.h"

#include "circt/Conversion/Passes.h"
#include "circt/Dialect/Emit/EmitDialect.h"
#include "circt/Dialect/FIRRTL/CHIRRTLDialect.h"
#include "circt/Dialect/FIRRTL/FIRParser.h"
#include "circt/Dialect/FIRRTL/FIRRTLDialect.h"
#include "circt/Dialect/FIRRTL/FIRRTLOps.h"
#include "circt/Dialect/FIRRTL/FIRRTLTypes.h"
#include "circt/Dialect/FIRRTL/FIRRTLUtils.h"
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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stack>
#include <unordered_set>
#include <vector>

using BitsPrimOp = circt::firrtl::BitsPrimOp;
using CHIRRTLDialect = circt::chirrtl::CHIRRTLDialect;
using Cell = eda::gate::model::Cell;
using CellID = eda::gate::model::CellID;
using CellProperties = eda::gate::model::CellProperties;
using CellSymbol = eda::gate::model::CellSymbol;
using CellType = eda::gate::model::CellType;
using CircuitOp = circt::firrtl::CircuitOp;
using CompanionMode = circt::firrtl::CompanionMode;
using ConstantOp = circt::firrtl::ConstantOp;
using DefaultTimingManager = mlir::DefaultTimingManager;
using DialectRegistry = mlir::DialectRegistry;
using Direction = circt::firrtl::Direction;
using EmitDialect = circt::emit::EmitDialect;
using FModuleOp = circt::firrtl::FModuleOp;
using FIRParserOptions = circt::firrtl::FIRParserOptions;
using FIRRTLDialect = circt::firrtl::FIRRTLDialect;
using LLVMStringLiteral = llvm::StringLiteral;
using LLVMStringRef = llvm::StringRef;
using LinkEnd = eda::gate::model::LinkEnd;
using LogicalResult = mlir::LogicalResult;
using MLIRContext = mlir::MLIRContext;
using ModuleOp = mlir::ModuleOp;
using NameKindEnum = circt::firrtl::NameKindEnum;
using Net = eda::gate::model::Net;
using NetBuilder = eda::gate::model::NetBuilder;
using OMDialect = circt::om::OMDialect;
template<typename OperationType>
using OpConversionPattern = mlir::OpConversionPattern<OperationType>;
using Operation = mlir::Operation;
template<typename Type = void>
using OperationPass = mlir::OperationPass<Type>;
using OpPassManager = mlir::OpPassManager;
template<typename Type>
using OwningOpRef = mlir::OwningOpRef<Type>;
using Pass = mlir::Pass;
using PassManager = mlir::PassManager;
using PortInfo = circt::firrtl::PortInfo;
using PropAssignOp = circt::firrtl::PropAssignOp;
using PropertyType = circt::firrtl::PropertyType;
using Region = mlir::Region;
using SVDialect = circt::sv::SVDialect;
using ShlPrimOp = circt::firrtl::ShlPrimOp;
using ShrPrimOp = circt::firrtl::ShrPrimOp;
using SourceMgr = llvm::SourceMgr;
using StrictConnectOp = circt::firrtl::StrictConnectOp;
using StringAttr = mlir::StringAttr;
using StringRef = mlir::StringRef;
using TypeID = mlir::TypeID;
using Type = mlir::Type;
using UIntType = circt::firrtl::UIntType;
using Value = mlir::Value;
using WireOp = circt::firrtl::WireOp;

namespace PreserveAggregate = circt::firrtl::PreserveAggregate;
namespace PreserveValues = circt::firrtl::PreserveValues;
namespace chirrtl = circt::chirrtl;
namespace firrtl = circt::firrtl;
namespace fs = std::filesystem;
namespace hw = circt::hw;
namespace model = eda::gate::model;
namespace om = circt::om;
namespace sv = circt::sv;

namespace eda::gate::translator {

std::vector<CellTypeID> getNetlist(const std::string &inputFilePath) {
  const fs::path inPath = inputFilePath;
  if (!fs::exists(inPath)) {
    std::cerr << "File does not exist: " << inputFilePath << "\n";
    return std::vector<CellTypeID>();
  }
  std::string extension = inPath.extension();
  if (extension != ".fir" && extension != ".mlir") {
    std::cerr << "Unsupported file type: " << extension << "\n";
    return std::vector<CellTypeID>();
  }

  // Parse the input 'FIRRTL' / 'MLIR' file.
  Translator translator{ extension == ".fir" ?
      MLIRModule::loadFromFIRFile(inputFilePath) :
      MLIRModule::loadFromMLIRFile(inputFilePath) };

#ifdef UTOPIA_DEBUG
  translator.printFIRRTL();
#endif // UTOPIA_DEBUG

  // Translate the 'FIRRTL' representation to the net.
  return translator.translate();
}

bool printNetlist(const std::vector<CellTypeID> netlist,
                  const std::string &outputFileName) {

  // Dump the output net to the console (Format::SIMPLE).
#ifdef UTOPIA_DEBUG
  for (const auto &cellTypeID : netlist) {
    std::cout << CellType::get(cellTypeID).getNet() << "\n";
  }
#endif // UTOPIA_DEBUG

  std::ofstream outputStream(outputFileName);
  for (const auto &cellTypeID : netlist) {
    const auto &net = CellType::get(cellTypeID).getNet();
    model::print(outputStream, model::VERILOG, net);
  }
  outputStream.close();
  return true;
}

bool printNetlist(const std::string &inputFilePath,
                  const std::string &outputDir) {
  const auto resultNetlist = getNetlist(inputFilePath);
  if (resultNetlist.empty()) {
    return false;
  }
  // Dump the output net to the '.v' file.
  fs::path outPath = fs::path(inputFilePath).filename();
  outPath.replace_extension(".v");
  fs::create_directories(outputDir);
  const fs::path outputFullName = outputDir / outPath;
  return printNetlist(resultNetlist, outputFullName.c_str());
}

MLIRModule MLIRModule::loadFromMLIR(const std::string &fileName,
                                    const std::string &string) {
  auto context = std::make_unique<MLIRContext>();
  context->getOrLoadDialect<FIRRTLDialect>();
  auto moduleOp = mlir::parseSourceString<ModuleOp>(string, context.get());

  uassert(moduleOp, "The input file " << fileName << " is incorrect!\n");

  return { std::move(context), std::move(moduleOp) };
}

MLIRModule MLIRModule::loadFromMLIRFile(const std::string &fileName) {
  std::ifstream file{ fileName };
  std::stringstream buf;

  uassert(file, "File " << fileName << " doesn't exist!\n");

  buf << file.rdbuf();
  
  return loadFromMLIR(fileName, buf.str());
}

MLIRModule MLIRModule::loadFromFIRFile(const std::string &fileName) {
  DefaultTimingManager tm;
  auto ts = tm.getRootScope();
  SourceMgr sourceMgr;
  auto file = mlir::openInputFile(fileName);

  uassert(file, "File " << fileName << " doesn't exist!\n");

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

  uassert(moduleOp, "File " << fileName << " is incorrect!\n");

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
  resultNetlist = std::make_unique<std::vector<CellTypeID>>();
}


void Translator::addPass(std::unique_ptr<Pass> pass) {
  passManager.addPass(std::move(pass));
}

void Translator::runPasses() {
  ModuleOp moduleOp = module.getRoot();
  if (failed(passManager.run(moduleOp))) {
    uassert(false, "Some passes failed!\n");
  }
}

void Translator::clearPasses() {
  passManager.clear();
}

void Translator::printFIRRTL() {
  std::string buf;
  llvm::raw_string_ostream os{buf};
  module.print(os);
  std::cout << buf << "\n";
}

std::vector<CellTypeID> Translator::translate() {
  addPass(createCHIRRTLToLowFIRRTLPass());
  runPasses();
  clearPasses();

#ifdef UTOPIA_DEBUG
  printFIRRTL();
#endif // UTOPIA_DEBUG

  addPass(createLowFIRRTLToNetPass(resultNetlist));
  runPasses();
  clearPasses();
  return *resultNetlist;
}

bool LinkKey::operator==(const LinkKey &linkKey) const {
  return (op == linkKey.op &&
          portNum == linkKey.portNum &&
          bitNum == linkKey.bitNum);
}

bool CellTypeKey::operator==(const CellTypeKey &cellTypeKey) const {
  if (name != cellTypeKey.name ||
      portWidthIn.size() != cellTypeKey.portWidthIn.size() ||
      portWidthOut.size() != cellTypeKey.portWidthOut.size()) {
    return false;
  }
  for (size_t i = 0; i < portWidthIn.size(); i++) {
    if (portWidthIn[i] != cellTypeKey.portWidthIn[i]) {
      return false;
    }
  }
  for (size_t i = 0; i < portWidthOut.size(); i++) {
    if (portWidthOut[i] != cellTypeKey.portWidthOut[i]) {
      return false;
    }
  }
  return true;
}
} // namespace eda::gate::translator

namespace {

struct LinkInfo {
  uint low;
  uint high;
  uint off;
  int bitOff;
};

struct OpInfo {
  Value value;
  Operation *op;
};

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
    registry.insert<EmitDialect>();
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

class LowFIRRTLToNetPass : public TranslatePass<LowFIRRTLToNetPass> {
private:
  std::shared_ptr<std::vector<CellTypeID>> resultNetlist;
  std::unordered_map<LinkKey, LinkEnd> linkKeyToLinkEndOuts;
  std::unordered_map<LinkKey, std::vector<CellID>> cellKeyToCellIDsIns;
  std::unordered_map<CellTypeKey, CellTypeID> cellTypeKeyToCellTypeIDs;

  void generateModel(ModuleOp moduleOp,
      std::shared_ptr<std::vector<CellTypeID>> resultNetlist);
  void processConnects(StrictConnectOp strictConnectOp, FModuleOp fModuleOp,
      NetBuilder *netBuilder,
      CellID &cellIDForZero, CellID &cellIDForOne);
  bool checkConnections();
  void processOperation(Operation *destOp, FModuleOp fModuleOp,
      NetBuilder *netBuilder,
      CellID &cellIDForZero, CellID &cellIDForOne);
  void processRegReset(RegResetOp regResetOp, FModuleOp fModuleOp,
      NetBuilder *netBuilder);
  void processReg(RegOp regOp, FModuleOp fModuleOp, NetBuilder *netBuilder);
  void processBoolLogicReduce(Operation *boolLogicRop, FModuleOp fModuleOp,
      NetBuilder *netBuilder);
  void processBoolLogic(Operation *boolLogiOp, FModuleOp fModuleOp,
      NetBuilder *netBuilder);
  void processBitManipulation(Operation *op, FModuleOp fModuleOp,
      NetBuilder *netBuilder,
      CellID &cellIDForZero);
  void processBits(BitsPrimOp bitsOp, const uint bitWidthIn,
      std::vector<LinkEnd> &linkEnds, std::vector<LinkEnd> &outLinkEnds);
  void processTail(TailPrimOp tailOp, const uint bitWidthIn,
      std::vector<LinkEnd> &linkEnds, std::vector<LinkEnd> &outLinkEnds);
  void processHead(HeadPrimOp headOp, std::vector<LinkEnd> &linkEnds,
      std::vector<LinkEnd> &outLinkEnds);
  void processShiftRight(ShrPrimOp shrOp, CellID &cellIDForZero,
      const uint bitWidthIn, std::vector<LinkEnd> &linkEnds,
      std::vector<LinkEnd> &outLinkEnds, NetBuilder *netBuilder);
  void processShiftLeft(ShlPrimOp shlOp, CellID &cellIDForZero,
      const uint bitWidthIn, std::vector<LinkEnd> &linkEnds,
      std::vector<LinkEnd> &outLinkEnds, NetBuilder *netBuilder);
  void processPad(PadPrimOp padOp, CellID &cellIDForZero, const uint bitWidthIn,
      std::vector<LinkEnd> &linkEnds, std::vector<LinkEnd> &outLinkEnds,
      NetBuilder *netBuilder);
  void processSynthesizable(Operation *synthOp, FModuleOp fModuleOp,
      NetBuilder *netBuilder);
  void processInstance(InstanceOp instOp, NetBuilder *netBuilder);
  void getToLinkKeysSynthOps(const Value &val, FModuleOp fModuleOp,
      std::vector<std::pair<uint,uint>> &fromLinkKeysMargins,
      std::vector<std::vector<LinkKey>> &toLinkKeys);
  void walkFinal(Operation *op, const uint opNumber,
      const uint typeWidth, LinkInfo &linkInfo,
      std::vector<std::pair<uint,uint>> &fromLinkKeysMargins,
      std::vector<std::vector<LinkKey>> &toLinkKeys);
  LinkInfo walkThroughPad(std::vector<Value> &bitManVals, PadPrimOp padOp,
      const LinkInfo &linkInfo);
  LinkInfo walkThroughShiftLeft(std::vector<Value> &bitManVals, ShlPrimOp shlOp,
      const LinkInfo &linkInfo);
  void walkThroughBits(std::vector<Value> &bitManVals, BitsPrimOp bitsOp,
      const LinkInfo &linkInfo, std::vector<LinkInfo> &linkInfoCol);
  LinkInfo walkThroughCat(std::vector<Value> &bitManVals, OpInfo curOpInfo,
      OpInfo prevOpInfo, uint opNum, const LinkInfo &linkInfo);
  void processWires(FModuleOp fModuleOp, NetBuilder *netBuilder);
  void generateOutputs(FModuleOp fModuleOp, NetBuilder *netBuilder);
  void generateInputs(FModuleOp fModuleOp, NetBuilder *netBuilder,
      CellID &cellIDForZero, CellID &cellIDForOne);
  std::vector<LinkEnd> getLinkEnds(Operation *destOp, FModuleOp fModuleOp);
public:
  LowFIRRTLToNetPass(
      std::shared_ptr<std::vector<CellTypeID>> resultNetlist) {
    this->resultNetlist = resultNetlist;
  }
  void runOnOperation() override {
    ModuleOp moduleOp = getOperation();
    generateModel(moduleOp, resultNetlist);
  }
};

std::vector<LinkEnd> LowFIRRTLToNetPass::getLinkEnds(Operation *destOp,
    FModuleOp fModuleOp) {
  std::vector<LinkEnd> linkEnds;
  uint operandCount = destOp->getNumOperands();
  for (uint i = 0; i < operandCount; i++) {
    const Value operand = getDestValue(destOp, i);
    Operation *srcOp = getSourceOperation(destOp, operand);
    const uint resNumber = findOpResultNumber(operand, srcOp, fModuleOp);
    const uint inWidth = getTypeWidth(operand.getType());

    for (uint j = 0; j < inWidth; j++) {
      const LinkKey srcKey(srcOp, resNumber, j);
      LinkEnd linkEndSrc = linkKeyToLinkEndOuts[srcKey];

      uassert((linkKeyToLinkEndOuts.count(srcKey) != 0),
              "No LinkEnds for a LinkKey have been found!");

      linkEnds.push_back(linkEndSrc);
    }
  }
  return linkEnds;
}

void LowFIRRTLToNetPass::generateInputs(FModuleOp fModuleOp,
    NetBuilder *netBuilder,
    CellID &cellIDForZero,
    CellID &cellIDForOne) {
  // Inputs.
  uint inNumber = 0;
  for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
    if (fModuleOp.getPortDirection(i) == Direction::In &&
        !(mlir::dyn_cast<PropertyType>(fModuleOp.getPortType(i)))) {
      uint portWidth = getTypeWidth(fModuleOp.getPortType(i));
      for (uint j = 0; j < portWidth; j++) {
        CellID cellID = makeCell(CellSymbol::IN);
        const LinkKey linkKey(nullptr, inNumber, j);
        linkKeyToLinkEndOuts.emplace(std::make_pair(linkKey, LinkEnd(cellID)));
        netBuilder->addCell(cellID);
      }
      inNumber++;
    }
  }
  // Constants.
  fModuleOp.walk([&](ConstantOp constantOp) {
    const uint outputWidth = getResultWidth(constantOp, 0);
    auto &&value = constantOp.getValue();
    for (size_t i = 0; i < outputWidth; i++) {
      const uint extractedBit = value.extractBitsAsZExtValue(1, i);
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
      const LinkKey linkKey(constantOp, 0, i);
      linkKeyToLinkEndOuts.emplace(std::make_pair(linkKey, LinkEnd(cellID)));
      netBuilder->addCell(cellID);
    }
  });
}

void LowFIRRTLToNetPass::generateOutputs(FModuleOp fModuleOp,
    NetBuilder *netBuilder) {
  size_t outNumber = 0;
  size_t inCount = 0;
  // The number of the inputs is needed for distinguishing the outputs from
  // the inputs when they appear as inputs in some operations.
  for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
    if (fModuleOp.getPortDirection(i) == Direction::In &&
        !(mlir::dyn_cast<PropertyType>(fModuleOp.getPortType(i)))) {
      inCount++;
    }
  }
  for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
    if (fModuleOp.getPortDirection(i) == Direction::Out &&
        !(mlir::dyn_cast<PropertyType>(fModuleOp.getPortType(i)))) {
      const uint portWidth = getTypeWidth(fModuleOp.getPortType(i));

      for (uint j = 0; j < portWidth; j++) {
        LinkEnd emptyLink;
        CellID cellOutID = makeCell(CellSymbol::OUT, emptyLink);
        netBuilder->addCell(cellOutID);
        LinkKey linkKeyIn(nullptr, outNumber, j);
        LinkKey linkKeyOut(nullptr, outNumber + inCount, j);
        std::vector<CellID> cellIDs;
        cellIDs.push_back(cellOutID);
        cellKeyToCellIDsIns.emplace(std::make_pair(linkKeyIn, cellIDs));
        linkKeyToLinkEndOuts.emplace(std::make_pair(linkKeyOut,
                                                    LinkEnd(cellOutID)));
      }
      outNumber++;
    }
  }
}

void LowFIRRTLToNetPass::processWires(FModuleOp fModuleOp,
    NetBuilder *netBuilder) {
  fModuleOp.walk([&](WireOp wireOp) {
    const uint outWidth = getResultWidth(wireOp, 0);
    for (size_t i = 0; i < outWidth; i++) {
      LinkKey linkKeyIn(wireOp, 0, i);
      LinkKey linkKeyOut(wireOp, 0, i);
      std::vector<CellID> cellIDs;
      cellIDs.push_back(model::OBJ_NULL_ID);
      cellKeyToCellIDsIns.emplace(std::make_pair(linkKeyIn, cellIDs));
      linkKeyToLinkEndOuts.emplace(std::make_pair(linkKeyOut, LinkEnd()));
    }
  });
}

LinkInfo LowFIRRTLToNetPass::walkThroughCat(std::vector<Value> &bitManVals,
    OpInfo curOpInfo, OpInfo prevOpInfo, uint opNum, const LinkInfo &linkInfo) {
  uint newOff = linkInfo.off;
  int newBitOff = linkInfo.bitOff;
  bitManVals.push_back(curOpInfo.op->getResult(0));
  if (prevOpInfo.op == curOpInfo.op && prevOpInfo.value == curOpInfo.value) {
    opNum++;
  }
  if (opNum == 1) {
    newOff += getOperandWidth(curOpInfo.op, 0);
  } else {
    newBitOff += getOperandWidth(curOpInfo.op, 1);
  }
  return {linkInfo.low, linkInfo.high, newOff, newBitOff};
}

void LowFIRRTLToNetPass::walkThroughBits(std::vector<Value> &bitManVals,
    BitsPrimOp bitsOp, const LinkInfo &linkInfo,
    std::vector<LinkInfo> &linkInfoCol) {
  const uint hi = bitsOp.getHi();
  const uint lo = bitsOp.getLo();
  uint newLow = std::max(lo, linkInfo.low + linkInfo.bitOff);
  uint newHigh = std::min(hi, linkInfo.high + linkInfo.bitOff);
  if (newHigh >= newLow) {
    newLow -= linkInfo.bitOff;
    newLow = (newLow < 0) ? 0 : newLow;
    newHigh -= linkInfo.bitOff;
    newHigh = (newHigh < 0) ? 0 : newHigh;
    const uint bitWidth = getOperandWidth(bitsOp, 0);
    const uint bitsCutFromLeft = (bitWidth - 1) - hi;
    uint newOff = linkInfo.off - (bitsCutFromLeft - (linkInfo.high - newHigh));
    const uint bitsCutFromRight = lo;
    int newBitOff = linkInfo.bitOff - bitsCutFromRight;
    bitManVals.push_back(bitsOp->getResult(0));
    linkInfoCol.push_back({newLow, newHigh, newOff, newBitOff});
  }
}

LinkInfo LowFIRRTLToNetPass::walkThroughShiftLeft(
    std::vector<Value> &bitManVals, ShlPrimOp shlOp,
    const LinkInfo &linkInfo) {
  int newBifOff = linkInfo.bitOff;
  bitManVals.push_back(shlOp->getResult(0));
  newBifOff += getOperandWidth(shlOp, 0);
  return {linkInfo.low, linkInfo.high, linkInfo.off, newBifOff};
}

LinkInfo LowFIRRTLToNetPass::walkThroughPad( std::vector<Value> &bitManVals,
    PadPrimOp padOp, const LinkInfo &linkInfo) {
  uint newOff = linkInfo.off;
  bitManVals.push_back(padOp->getResult(0));
  const uint numBitsPadded = padOp.getAmount() - getOperandWidth(padOp, 0);
  newOff += numBitsPadded;
  return {linkInfo.low, linkInfo.high, newOff, linkInfo.bitOff};
}

void LowFIRRTLToNetPass::walkFinal(Operation *op, const uint opNum,
    const uint typeWidth, LinkInfo &linkInfo,
    std::vector<std::pair<uint,uint>> &fromLinkKeysMargins,
    std::vector<std::vector<LinkKey>> &toLinkKeys) {
  std::vector<LinkKey> toLinkKeysSimple;
  // Reversing the margins because of the difference in endianness.
  const uint lowRev = typeWidth - linkInfo.high - 1;
  const uint highRev = typeWidth - linkInfo.low;
  for (uint i = 0; i < highRev - lowRev; i++) {
    toLinkKeysSimple.push_back(LinkKey(op, opNum, linkInfo.off + i));
  }
  toLinkKeys.push_back(toLinkKeysSimple);
  fromLinkKeysMargins.push_back({ lowRev, highRev });
}

void LowFIRRTLToNetPass::getToLinkKeysSynthOps(const Value &val,
    FModuleOp fModuleOp, std::vector<std::pair<uint,uint>> &fromLinkKeysMargins,
    std::vector<std::vector<LinkKey>> &toLinkKeys) {
  std::vector<Value> bitManVals;
  bitManVals.push_back(val);
  std::vector<LinkInfo> linkInfoCol;
  uint typeWidth = getTypeWidth(val.getType());
  linkInfoCol.push_back({0, typeWidth - 1, 0, 0});
  Operation* prevOp;
  Value prevVal;
  while(!bitManVals.empty()) {
    Value curVal = bitManVals.back();
    bitManVals.pop_back();
    LinkInfo linkInfo = linkInfoCol.back();
    linkInfoCol.pop_back();
    for (auto *user : curVal.getUsers()) {
      if (auto connect = mlir::dyn_cast<FConnectLike>(user)) {
        if (connect.getSrc() == curVal) {
          Value nextVal = connect.getDest();
          Operation *nextOp = nextVal.getDefiningOp();
          if (nextOp && (isWire(nextOp) || isSimpleLinkMove(nextOp))) {
            bitManVals.push_back(nextVal);
            linkInfoCol.push_back(linkInfo);
          } else {
            uint opNum = findOpOperandNumber(nextVal, nextOp, fModuleOp);
            walkFinal(nextOp, opNum, typeWidth, linkInfo,
                      fromLinkKeysMargins, toLinkKeys);
          }
        }
      } else {
        uint opNum = findOpOperandNumber(curVal, user, fModuleOp);
        if (isBits(user)) {
          auto bitsOp = mlir::dyn_cast<BitsPrimOp>(user);
          walkThroughBits(bitManVals, bitsOp, linkInfo, linkInfoCol);
        } else if (isBitManipulation(user)) {
          LinkInfo newLinkInfo = linkInfo;
          if (isConcatenation(user)) {
            auto catOp = mlir::dyn_cast<CatPrimOp>(user);
            newLinkInfo = walkThroughCat(bitManVals, {curVal, catOp},
                {prevVal, prevOp}, opNum, linkInfo);
          } else if (isShiftLeft(user)) {
            auto shlOp = mlir::dyn_cast<ShlPrimOp>(user);
            newLinkInfo = walkThroughShiftLeft(bitManVals, shlOp, linkInfo);
          } else if (isPad(user)) {
            auto padOp = mlir::dyn_cast<PadPrimOp>(user);
            newLinkInfo = walkThroughPad(bitManVals, padOp, linkInfo);
          } else if (isSimpleLinkMove(user)) {
            bitManVals.push_back(user->getResult(0));
          }
          linkInfoCol.push_back(newLinkInfo);
        } else {
          walkFinal(user, opNum, typeWidth, linkInfo,
                    fromLinkKeysMargins, toLinkKeys);
        }
      }
      prevOp = user;
      prevVal = curVal;
    }
  }
}

void LowFIRRTLToNetPass::processInstance(InstanceOp instOp,
    NetBuilder *netBuilder) {
  const std::vector<uint16_t> portWidthIn = getPortWidthIn(instOp);
  const std::vector<uint16_t> portWidthOut = getPortWidthOut(instOp);
  const CellSymbol cellSymbol = getCellSymbol(instOp);
  const std::string &cellTypeName = instOp.getModuleName().str();
  CellTypeKey cellTypeKey(cellTypeName, portWidthIn, portWidthOut);
  CellTypeID cellTypeID;
  if (cellTypeKeyToCellTypeIDs.count(cellTypeKey) != 0) {
    cellTypeID = cellTypeKeyToCellTypeIDs[cellTypeKey];
  } else {
    cellTypeID = makeSoftType(cellSymbol,
                              cellTypeName,
                              model::OBJ_NULL_ID,
                              portWidthIn,
                              portWidthOut);
  }
  std::vector<LinkEnd> linkEnds;
  for (uint i = 0; i < portWidthIn.size(); i++) {
    for (uint16_t j = 0; j < portWidthIn[i]; j++) {
      linkEnds.push_back(LinkEnd());
    }
  }
  CellID cellDestID = makeCell(cellTypeID, linkEnds);
  netBuilder->addCell(cellDestID);
  uint outNum = 0;
  uint inNum = 0;
  for (uint i = 0; i < instOp->getNumResults(); i++) {
    auto &&result = instOp->getResult(i);
    const uint width = getTypeWidth(result.getType());
    if (instOp.getPortDirection(i) == Direction::Out) {
      for (uint j = 0; j < width; j++) {
        const LinkKey outKey(instOp, outNum, j);
        linkKeyToLinkEndOuts.emplace(std::make_pair(outKey,
                                                    LinkEnd(cellDestID)));
      }
      outNum++;
    } else {
      for (uint j = 0; j < width; j++) {
        const LinkKey inKey(instOp, inNum, j);
        std::vector<CellID> cellIDs;
        cellIDs.push_back(cellDestID);
        cellKeyToCellIDsIns.emplace(std::make_pair(inKey, cellIDs));
      }
      inNum++;
    }
  }
}

void LowFIRRTLToNetPass::processSynthesizable(Operation *synthOp,
    FModuleOp fModuleOp, NetBuilder *netBuilder) {
  const std::vector<uint16_t> portWidthIn = getPortWidthIn(synthOp);
  const std::vector<uint16_t> portWidthOut = getPortWidthOut(synthOp);
  const CellSymbol cellSymbol = getCellSymbol(synthOp);
  auto linkEnds = getLinkEnds(synthOp, fModuleOp);
  const std::string &cellTypeName = synthOp->getName().stripDialect().str();
  CellTypeKey cellTypeKey(cellTypeName, portWidthIn, portWidthOut);
  CellTypeID cellTypeID;
  if (cellTypeKeyToCellTypeIDs.count(cellTypeKey) != 0) {
    cellTypeID = cellTypeKeyToCellTypeIDs[cellTypeKey];
  } else {
    cellTypeID = makeSoftType(cellSymbol,
                              cellTypeName,
                              model::OBJ_NULL_ID,
                              portWidthIn,
                              portWidthOut);
  }
  CellID cellDestID = makeCell(cellTypeID, linkEnds);
  netBuilder->addCell(cellDestID);
  const uint inCount = getInCount(synthOp);
  for (uint i = 0; i < inCount; i++) {
    auto &&arg = synthOp->getOperand(i);
    uint inWidth = getTypeWidth(arg.getType());
    for (uint j = 0; j < inWidth; j++) {
      LinkKey inKey(synthOp, i, j);
      std::vector<CellID> cellIDs;
      cellIDs.push_back(cellDestID);
      cellKeyToCellIDsIns.emplace(std::make_pair(inKey, cellIDs));
    }
  }
  const uint outCount = getOutCount(synthOp);
  for (uint i = 0; i < outCount; i++) {
    auto &&result = synthOp->getResult(i);
    uint outWidth = getTypeWidth(result.getType());
    for (uint j = 0; j < outWidth; j++) {
      LinkKey outKey(synthOp, i, j);
      linkKeyToLinkEndOuts.emplace(std::make_pair(outKey, LinkEnd(cellDestID)));
    }
  }
}

void LowFIRRTLToNetPass::processPad(PadPrimOp padOp, CellID &cellIDForZero,
    const uint bitWidthIn, std::vector<LinkEnd> &linkEnds,
    std::vector<LinkEnd> &outLinkEnds, NetBuilder *netBuilder) {
  auto &&argument = padOp->getOperand(0);
  auto &&type = argument.getType();
  const uint amount = padOp.getAmount();
  if (amount > bitWidthIn) {
    uint numberOfPads = amount - bitWidthIn;
    if (circt::firrtl::type_isa<UIntType>(type)) {
      if (cellIDForZero == model::OBJ_NULL_ID) {
        cellIDForZero = makeCell(CellSymbol::ZERO);
        netBuilder->addCell(cellIDForZero);
      }
      for (uint i = 0; i < numberOfPads; i++) {
        outLinkEnds.push_back(LinkEnd(cellIDForZero));
      }
    } else if (circt::firrtl::type_isa<IntType>(type)) {
      for (uint i = 0; i < numberOfPads; i++) {
        outLinkEnds.push_back(LinkEnd(linkEnds[0]));
      }
    }
    for (uint i = 0; i < bitWidthIn; i++) {
      outLinkEnds.push_back(linkEnds[i]);
    }
  } else {
    for (uint i = 0; i < bitWidthIn; i++) {
      outLinkEnds.push_back(linkEnds[i]);
    }
  }
}

void LowFIRRTLToNetPass::processShiftLeft(ShlPrimOp shlOp,
    CellID &cellIDForZero, const uint bitWidthIn,
    std::vector<LinkEnd> &linkEnds, std::vector<LinkEnd> &outLinkEnds,
    NetBuilder *netBuilder) {
  if (cellIDForZero == model::OBJ_NULL_ID) {
    cellIDForZero = makeCell(CellSymbol::ZERO);
    netBuilder->addCell(cellIDForZero);
  }
  for (uint i = 0; i < bitWidthIn; i++) {
    outLinkEnds.push_back(linkEnds[i]);
  }
  uint amount = shlOp.getAmount();
  for (uint i = 0; i < amount; i++) {
    outLinkEnds.push_back(LinkEnd(cellIDForZero));
  }
}

void LowFIRRTLToNetPass::processShiftRight(ShrPrimOp shrOp,
    CellID &cellIDForZero, const uint bitWidthIn,
    std::vector<LinkEnd> &linkEnds, std::vector<LinkEnd> &outLinkEnds,
    NetBuilder *netBuilder) {
  auto &&argument = shrOp->getOperand(0);
  auto &&type = argument.getType();
  const uint amount = shrOp.getAmount();
  if (amount >= bitWidthIn) {
    if (circt::firrtl::type_isa<UIntType>(type)) {
      if (cellIDForZero == model::OBJ_NULL_ID) {
        cellIDForZero = makeCell(CellSymbol::ZERO);
        netBuilder->addCell(cellIDForZero);
        outLinkEnds.push_back(LinkEnd(cellIDForZero));
      }
    } else if (circt::firrtl::type_isa<IntType>(type)) {
      outLinkEnds.push_back(LinkEnd(linkEnds[0]));
    }
  } else {
    for (uint i = 0; i < amount; i++) {
      outLinkEnds.push_back(linkEnds[i]);
    }
  }
}

void LowFIRRTLToNetPass::processHead(HeadPrimOp headOp,
    std::vector<LinkEnd> &linkEnds, std::vector<LinkEnd> &outLinkEnds) {
  const uint amount = headOp.getAmount();
  for (uint i = 0; i < amount; i++) {
    outLinkEnds.push_back(linkEnds[i]);
  }
}

void LowFIRRTLToNetPass::processTail(TailPrimOp tailOp, const uint bitWidthIn,
    std::vector<LinkEnd> &linkEnds, std::vector<LinkEnd> &outLinkEnds) {
  const uint amount = tailOp.getAmount();
  for (uint i = 0; i < amount; i++) {
    outLinkEnds.push_back(linkEnds[bitWidthIn - amount + i]);
  }
}

void LowFIRRTLToNetPass::processBits(BitsPrimOp bitsOp, const uint bitWidthIn,
    std::vector<LinkEnd> &linkEnds, std::vector<LinkEnd> &outLinkEnds) {
  const uint lowMargin = bitsOp.getLo();
  const uint highMargin = bitsOp.getHi();

  // Reversing the margins because of the difference in endianness.
  const uint lowRev = bitWidthIn - highMargin - 1;
  const uint highRev = bitWidthIn - lowMargin;

  for (uint i = lowRev; i < highRev; i++) {
    outLinkEnds.push_back(linkEnds[i]);
  }
}

void LowFIRRTLToNetPass::processBitManipulation(Operation *op, 
    FModuleOp fModuleOp, NetBuilder *netBuilder, CellID &cellIDForZero) {
  auto linkEnds = getLinkEnds(op, fModuleOp);
  std::vector<LinkEnd> outLinkEnds;
  const std::vector<uint16_t> portWidthIn = getPortWidthIn(op);
  if (isPad(op)) {
    auto padOp = mlir::dyn_cast<PadPrimOp>(op);
    processPad(padOp, cellIDForZero, portWidthIn.back(), linkEnds, outLinkEnds,
               netBuilder);
  } else if (isShiftLeft(op)) {
    auto shlOp = mlir::dyn_cast<ShlPrimOp>(op);
    processShiftLeft(shlOp, cellIDForZero, portWidthIn.back(), linkEnds,
                     outLinkEnds, netBuilder);
  } else if (isShiftRight(op)) {
    auto shrOp = mlir::dyn_cast<ShrPrimOp>(op);
    processShiftRight(shrOp, cellIDForZero, portWidthIn.back(), linkEnds,
                      outLinkEnds, netBuilder);
  } else if (isConcatenation(op) || isSimpleLinkMove(op)) {
    uint count = 0;
    for (uint i = 0; i < portWidthIn.size(); i++) {
      for (uint j = 0; j < portWidthIn[i]; j++) {
        outLinkEnds.push_back(linkEnds[count++]);
      }
    }
  } else if (isHead(op)) {
    auto headOp = mlir::dyn_cast<HeadPrimOp>(op);
    processHead(headOp, linkEnds, outLinkEnds);
  } else if (isTail(op)) {
    auto tailOp = mlir::dyn_cast<TailPrimOp>(op);
    processTail(tailOp, portWidthIn.back(), linkEnds, outLinkEnds);
  } else if (isBits(op)) {
    auto bitsOp = mlir::dyn_cast<BitsPrimOp>(op);
    processBits(bitsOp, portWidthIn.back(), linkEnds, outLinkEnds);
  }
  uint outLinkEndNum = 0;
  const uint outCount = getOutCount(op);
  for (uint i = 0; i < outCount; i++) {
    auto &&result = op->getResult(i);
    const uint outWidth = getTypeWidth(result.getType());
    for (uint j = 0; j < outWidth; j++) {
      const LinkKey destKey(op, i, j);
      linkKeyToLinkEndOuts.emplace(std::make_pair(destKey,
          outLinkEnds[outLinkEndNum++]));
    }
  }
}

void LowFIRRTLToNetPass::processBoolLogic(Operation *boolLogicOp,
    FModuleOp fModuleOp, NetBuilder *netBuilder) {
  auto linkEnds = getLinkEnds(boolLogicOp, fModuleOp);
  const CellSymbol cellSymbol = getCellSymbol(boolLogicOp);
  const CellTypeID cellTypeID = getCellTypeID(cellSymbol);
  const uint dataWidth = getResultWidth(boolLogicOp, 0);
  const uint inCount = getInCount(boolLogicOp);
  for (uint j = 0; j < dataWidth; j++) {
    std::vector<LinkEnd> linkEndsForOne;
    for (uint i = 0; i < inCount; i++) {
      linkEndsForOne.push_back(linkEnds[i * dataWidth + j]);
    }
    CellID cellDestID = makeCell(cellTypeID, linkEndsForOne);
    netBuilder->addCell(cellDestID);
    const LinkKey destKey(boolLogicOp, 0, j);
    linkKeyToLinkEndOuts.emplace(std::make_pair(destKey, LinkEnd(cellDestID)));
    const LinkKey firstArgKey(boolLogicOp, 0, j);
    std::vector<CellID> cellIDs0;
    cellIDs0.push_back(cellDestID);
    cellKeyToCellIDsIns.emplace(std::make_pair(firstArgKey, cellIDs0));
    const LinkKey secondArgKey(boolLogicOp, 1, j);
    std::vector<CellID> cellIDs1;
    cellIDs1.push_back(cellDestID);
    cellKeyToCellIDsIns.emplace(std::make_pair(secondArgKey, cellIDs1));
  }
}

void LowFIRRTLToNetPass::processBoolLogicReduce(Operation *boolLogicRop,
    FModuleOp fModuleOp, NetBuilder *netBuilder) {
  auto linkEnds = getLinkEnds(boolLogicRop, fModuleOp);
  const CellSymbol cellSymbol = getCellSymbol(boolLogicRop);
  const CellTypeID cellTypeID = getCellTypeID(cellSymbol);
  CellID cellDestID = makeCell(cellTypeID, linkEnds);
  netBuilder->addCell(cellDestID);
  const LinkKey destKey(boolLogicRop, 0, 0);
  linkKeyToLinkEndOuts.emplace(std::make_pair(destKey, LinkEnd(cellDestID)));
  const uint dataWidth = getOperandWidth(boolLogicRop, 0);
  for (uint j = 0; j < dataWidth; j++) {
    LinkKey key(boolLogicRop, 0, j);
    std::vector<CellID> cellIDs;
    cellIDs.push_back(cellDestID);
    cellKeyToCellIDsIns.emplace(std::make_pair(key, cellIDs));
  }
}

void LowFIRRTLToNetPass::processReg(RegOp regOp, FModuleOp fModuleOp,
    NetBuilder *netBuilder) {
  auto linkEnds = getLinkEnds(regOp, fModuleOp);
  const CellSymbol cellSymbol = getCellSymbol(regOp);
  const CellTypeID cellTypeID = getCellTypeID(cellSymbol);
  const uint dataWidth = getResultWidth(regOp, 0);
  std::vector<CellID> cellIDsForClk;
  for (uint j = 0; j < dataWidth; j++) {
    // DFF(q, d, clk).
    std::vector<LinkEnd> linkEndsForOne;
    linkEndsForOne.push_back(LinkEnd());
    linkEndsForOne.push_back(linkEnds.front());
    CellID cellDestID = makeCell(cellTypeID, linkEndsForOne);
    netBuilder->addCell(cellDestID);
    const LinkKey outKey(regOp, 0, j);
    linkKeyToLinkEndOuts.emplace(std::make_pair(outKey, LinkEnd(cellDestID)));
    const LinkKey inKey(regOp, 0, j);
    std::vector<CellID> cellIDs;
    cellIDs.push_back(cellDestID);
    cellKeyToCellIDsIns.emplace(std::make_pair(inKey, cellIDs));
    cellIDsForClk.push_back(cellDestID);
  }
  const LinkKey inClkKey(regOp, 1, 0);
  cellKeyToCellIDsIns.emplace(std::make_pair(inClkKey, cellIDsForClk));
}

void LowFIRRTLToNetPass::processRegReset(RegResetOp regResetOp,
    FModuleOp fModuleOp, NetBuilder *netBuilder) {
  auto linkEnds = getLinkEnds(regResetOp, fModuleOp);
  const CellSymbol cellSymbol = getCellSymbol(regResetOp);
  const CellTypeID cellTypeID = getCellTypeID(cellSymbol);
  const uint dataWidth = getResultWidth(regResetOp, 0);
  const uint resetValueWidth = getOperandWidth(regResetOp, 2);
  std::vector<CellID> cellIDsForClk;
  std::vector<CellID> cellIDsForRst;
  std::vector<CellID> cellIDsForSet;
  for (uint j = 0, k = 0; j < dataWidth; j++) {
    // DFFrs(q, d, clk, rst, set).
    std::vector<LinkEnd> linkEndsForOne;
    linkEndsForOne.push_back(LinkEnd());
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
    const LinkKey outKey(regResetOp, 0, j);
    linkKeyToLinkEndOuts.emplace(std::make_pair(outKey, LinkEnd(cellDestID)));
    const LinkKey inKey(regResetOp, 0, j);
    std::vector<CellID> cellIDs;
    cellIDs.push_back(cellDestID);
    cellKeyToCellIDsIns.emplace(std::make_pair(inKey, cellIDs));
    cellIDsForClk.push_back(cellDestID);
    cellIDsForRst.push_back(negMidID);
    cellIDsForSet.push_back(andMidSID);
    // If a reset value doesn't have got a width of one - it's width must be
    // equal to the width of the data.
    if (resetValueWidth != 1) {
      k++;
    }
  }
  const LinkKey inClkKey(regResetOp, 1, 0);
  cellKeyToCellIDsIns.emplace(std::make_pair(inClkKey, cellIDsForClk));
  const LinkKey inRstKey(regResetOp, 2, 0);
  cellKeyToCellIDsIns.emplace(std::make_pair(inRstKey, cellIDsForRst));
  const LinkKey inSetKey(regResetOp, 3, 0);
  cellKeyToCellIDsIns.emplace(std::make_pair(inSetKey, cellIDsForSet));
}

void LowFIRRTLToNetPass::processOperation(Operation *destOp,
    FModuleOp fModuleOp, NetBuilder *netBuilder, CellID &cellIDForZero,
    CellID &cellIDForOne) {
  if (!isOmitted(destOp)) {
    if (isInstance(destOp)) {
      auto instOp = mlir::dyn_cast<InstanceOp>(destOp);
      processInstance(instOp, netBuilder);
    } else if (isSynthesizable(destOp)) {
      processSynthesizable(destOp, fModuleOp, netBuilder);
    } else if (isBitManipulation(destOp)) {
      processBitManipulation(destOp, fModuleOp, netBuilder, cellIDForZero);
    } else if (isBoolLogic(destOp)) {
      processBoolLogic(destOp, fModuleOp, netBuilder);
    } else if (isBoolLogicReduce(destOp)) {
      processBoolLogicReduce(destOp, fModuleOp, netBuilder);
    } else if (isRegister(destOp)) {
      auto regOp = mlir::dyn_cast<RegOp>(destOp);
      processReg(regOp, fModuleOp, netBuilder);
    } else if (isRegisterWithReset(destOp)) {
      auto regResetOp = mlir::dyn_cast<RegResetOp>(destOp);
      processRegReset(regResetOp, fModuleOp, netBuilder);
    } else {
      const std::string &destOpName = destOp->getName().getIdentifier().str();
      uassert(false, "Invalid operation in 'LowFIRRTL' code:" << destOpName <<
              "!\n");
    }
  }
}

void LowFIRRTLToNetPass::processConnects(StrictConnectOp strictConnectOp,
    FModuleOp fModuleOp, NetBuilder *netBuilder, CellID &cellIDForZero,
    CellID &cellIDForOne) {
  Value fromValue = strictConnectOp.getSrc();
  Value toValue = strictConnectOp.getDest();
  Operation *fromOp = fromValue.getDefiningOp();
  if (fromOp && isWire(fromOp)) {
    return;
  }
  Operation *toOp = toValue.getDefiningOp();
  std::vector<std::vector<LinkKey>> toLinkKeys;
  std::vector<std::pair<uint, uint>> fromLinkKeysMargins;
  Type type = fromValue.getType();
  uint typeWidth = getTypeWidth(type);
  if (toOp && (isWire(toOp) || isBitManipulation(toOp))) {
    getToLinkKeysSynthOps(toValue, fModuleOp, fromLinkKeysMargins, toLinkKeys);
  } else {
    const uint inPortNum = findOpOperandNumber(toValue, toOp, fModuleOp);
    std::vector<LinkKey> toLinkKeysSimple;
    for (uint i = 0; i < typeWidth; i++) {
      toLinkKeysSimple.push_back(LinkKey(toOp, inPortNum, i));
    }
    toLinkKeys.push_back(toLinkKeysSimple);
    fromLinkKeysMargins.push_back({ 0, typeWidth - 1 });
  }
  std::vector<LinkKey> fromLinkKeys;
  uint outPortNum = findOpResultNumber(fromValue, fromOp, fModuleOp);
  LinkKey srcKey(fromOp, outPortNum, 0);
  LinkEnd linkEndSrc = linkKeyToLinkEndOuts[srcKey];
  if ((Cell::get(linkEndSrc.getCellID())).isOut()) {
    fromValue = circt::firrtl::getModuleScopedDriver(fromValue, true, true,
                                                     false);
    fromOp = fromValue.getDefiningOp();
    type = fromValue.getType();
    typeWidth = getTypeWidth(type);
    outPortNum = findOpResultNumber(fromValue, fromOp, fModuleOp);
  }
  for (uint i = 0; i < typeWidth; i++) {
    const LinkKey srcKey(fromOp, outPortNum, i);
    fromLinkKeys.push_back(srcKey);
  }
  uint outerIndex = 0;
  for (const auto &toLinkKeysSimple : toLinkKeys) {
    uint innerIndex = fromLinkKeysMargins[outerIndex].first;
    for (const auto &toLinkKey : toLinkKeysSimple) {
      LinkEnd linkEndSrc = linkKeyToLinkEndOuts[fromLinkKeys[innerIndex]];
      if (!linkEndSrc.isValid()) {
        innerIndex++;
        continue;
      }
      const uint netInPortNum = getNetInPortNum(toLinkKey.op,
          toLinkKey.portNum, toLinkKey.bitNum);
      auto &destCellIDs = cellKeyToCellIDsIns[toLinkKey];
      for (const auto &destCellID : destCellIDs) {
        netBuilder->connect(destCellID, netInPortNum, linkEndSrc);
      }
      destCellIDs.clear();
      innerIndex++;
    }
    outerIndex++;
  }
}

// Top-level operation.
void LowFIRRTLToNetPass::generateModel(ModuleOp moduleOp,
    std::shared_ptr<std::vector<CellTypeID>> resultNetlist) {
  CircuitOp circuitOp =
      *(moduleOp.getRegion().begin()->getOps<CircuitOp>().begin());
  auto circuitName = circuitOp.getName().str();
  uint moduleCount = 0;
  for (auto &&fModuleOp : circuitOp.getBodyBlock()->getOps<FModuleOp>()) {
    NetBuilder netBuilder;
    CellID cellIDForZero = model::OBJ_NULL_ID;
    CellID cellIDForOne = model::OBJ_NULL_ID;
    generateInputs(fModuleOp, &netBuilder, cellIDForZero, cellIDForOne);
    generateOutputs(fModuleOp, &netBuilder);
    processWires(fModuleOp, &netBuilder);
    fModuleOp.walk([&](Operation *destOp) {
      processOperation(destOp, fModuleOp, &netBuilder,
                       cellIDForZero, cellIDForZero);
    });
    fModuleOp.walk([&](StrictConnectOp strictConnectOp) {
      processConnects(strictConnectOp, fModuleOp, &netBuilder,
                      cellIDForZero, cellIDForZero);
    });

    uassert(checkConnections(), "Some cells remain not fully connected!\n");

    NetID netID = netBuilder.make();
    const std::string &cellName = fModuleOp.getName().str();
    CellTypeID cellTypeID = makeSoftType(CellSymbol::UNDEF,
        cellName, netID, getModulePortWidths(fModuleOp, Direction::In),
        getModulePortWidths(fModuleOp, Direction::Out));
    if (cellName == circuitName && moduleCount != 0) {
      CellTypeID cellTypeIDBuf = resultNetlist->at(0);
      (*resultNetlist)[0] = cellTypeID;
      resultNetlist->push_back(cellTypeIDBuf);
    } else {
      resultNetlist->push_back(cellTypeID);
    }
    moduleCount++;
    linkKeyToLinkEndOuts.clear();
    cellKeyToCellIDsIns.clear();
  }
}

bool LowFIRRTLToNetPass::checkConnections() {
  for (const auto &[cellKey, cellIDIns] : cellKeyToCellIDsIns) {
    Operation* op = cellKey.op;
    // For wires keys with empty dummies were created.
    if (op && isWire(op)) {
      continue;
    }
#ifdef UTOPIA_DEBUG
    const uint portNum = cellKey.portNum;
    const uint bitNum = cellKey.bitNum;
    std::cout << "Cell key:\n";
    std::cout << op << " " << portNum << " " << bitNum << "\n";
    if (op) {
      std::cout << op->getName().getIdentifier().str() << "\n";
    }
    std::cout << "Cell IDs:\n";
#endif // UTOPIA_DEBUG
    for (const auto &cellID : cellIDIns) {
#ifdef UTOPIA_DEBUG
      std::cout << cellID << "\n";
#endif // UTOPIA_DEBUG
      const auto& cell = Cell::get(cellID);
      for (uint i = 0; i < cell.getFanin(); i++) {
        if (!(cell.getLink(i)).isValid()) {
          return false;
        }
      }
    }
  }
  return true;
}

} // namespace

std::unique_ptr<Pass> createCHIRRTLToLowFIRRTLPass() {
  return std::make_unique<CHIRRTLToLowFIRRTLPass>();
}

std::unique_ptr<Pass> createLowFIRRTLToNetPass(
    std::shared_ptr<std::vector<CellTypeID>> resultNetlist) {
  return std::make_unique<LowFIRRTLToNetPass>(resultNetlist);
}
