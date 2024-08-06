//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "fir_net.h"
#include "fir_net_utils.h"

#include "gate/model/celltype.h"
#include "gate/model/object.h"
#include "gate/model/printer/printer.h"
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
using Format = eda::gate::model::ModelPrinter::Format;
using LLVMStringLiteral = llvm::StringLiteral;
using LLVMStringRef = llvm::StringRef;
using LinkEnd = eda::gate::model::LinkEnd;
using LogicalResult = mlir::LogicalResult;
using MLIRContext = mlir::MLIRContext;
using ModelPrinter = eda::gate::model::ModelPrinter;
using ModuleOp = mlir::ModuleOp;
using NameKindEnum = circt::firrtl::NameKindEnum;
using NetBuilder = eda::gate::model::NetBuilder;
using Net = eda::gate::model::Net;
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

std::vector<CellTypeID> getNet(const std::string &inputFilePath) {
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

bool printNet(const std::vector<CellTypeID> netlist,
              const std::string &outputFileName) {

  // Dump the output net to the console (Format::SIMPLE).
#ifdef UTOPIA_DEBUG
  for (const auto &cellTypeID : netlist) {
    std::cout << CellType::get(cellTypeID).getNet() << "\n";
  }
#endif // UTOPIA_DEBUG

  std::ofstream outputStream(outputFileName);
  for (const auto &cellTypeID : netlist) {
    ModelPrinter::getPrinter(Format::VERILOG).print(outputStream,
        CellType::get(cellTypeID).getNet());
  }
  outputStream.close();
  return true;
}

bool printNet(const std::string &inputFilePath,
              const std::string &outputDir) {
  const auto resultNetlist = getNet(inputFilePath);
  if (resultNetlist.empty()) {
    return false;
  }
  // Dump the output net to the '.v' file.
  fs::path outPath = fs::path(inputFilePath).filename();
  outPath.replace_extension(".v");
  fs::create_directories(outputDir);
  const fs::path outputFullName = outputDir / outPath;
  return printNet(resultNetlist, outputFullName.c_str());
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
  return (name == cellTypeKey.name &&
          bitWidthIn == cellTypeKey.bitWidthIn &&
          bitWidthOut == cellTypeKey.bitWidthOut);
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
  void walkThroughPad(std::vector<Value> &wireOrBitManValues, PadPrimOp padOp,
      LinkInfo &linkInfo, std::vector<LinkInfo> &linkInfoCol);
  void walkThroughShiftLeft(std::vector<Value> &wireOrBitManValues,
      ShlPrimOp shlOp, LinkInfo &linkInfo, std::vector<LinkInfo> &linkInfoCol);
  void walkThroughBits(std::vector<Value> &wireOrBitManValues,
      BitsPrimOp bitsOp, LinkInfo &linkInfo,
      std::vector<LinkInfo> &linkInfoCol);
  void walkThroughCat(std::vector<Value> &wireOrBitManValues, OpInfo curOpInfo,
      OpInfo prevOpInfo, uint opNumber, LinkInfo &linkInfo,
      std::vector<LinkInfo> &linkInfoCol);
  void processWires(FModuleOp fModuleOp, NetBuilder *netBuilder);
  void generateOutputs(FModuleOp fModuleOp, NetBuilder *netBuilder);
  void generateInputs(FModuleOp fModuleOp,
      NetBuilder *netBuilder,
      CellID &cellIDForZero,
      CellID &cellIDForOne);
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
    const uint outputWidth = getTypeWidth(constantOp.getResult().getType());
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
    const uint outWidth = getTypeWidth(wireOp.getResult().getType());
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

void LowFIRRTLToNetPass::walkThroughCat(std::vector<Value> &wireOrBitManValues,
    OpInfo curOpInfo, OpInfo prevOpInfo, uint opNumber, LinkInfo &linkInfo,
    std::vector<LinkInfo> &linkInfoCol) {
  wireOrBitManValues.push_back(curOpInfo.op->getResult(0));
  if (prevOpInfo.op == curOpInfo.op && prevOpInfo.value == curOpInfo.value) {
    linkInfo.bitOff -= getTypeWidth(curOpInfo.op->getOperand(1).getType());
    opNumber++;
  }
  if (opNumber == 1) {
    linkInfo.off += getTypeWidth(curOpInfo.op->getOperand(0).getType());
  } else {
    linkInfo.bitOff += getTypeWidth(curOpInfo.op->getOperand(1).getType());
  }
}

void LowFIRRTLToNetPass::walkThroughBits(std::vector<Value> &wireOrBitManValues,
    BitsPrimOp bitsOp, LinkInfo &linkInfo, std::vector<LinkInfo> &linkInfoCol) {
  wireOrBitManValues.push_back(bitsOp->getResult(0));
  const uint hi = bitsOp.getHi();
  const uint lo = bitsOp.getLo();
  uint newLowMargin = std::max(lo, linkInfo.low + linkInfo.bitOff);
  uint newHighMargin = std::min(hi, linkInfo.high + linkInfo.bitOff);
  if (newHighMargin >= newLowMargin) {
    // Offsetting back to old margins.
    newLowMargin -= linkInfo.bitOff;
    newLowMargin = (newLowMargin < 0) ? 0 : newLowMargin;
    newHighMargin -= linkInfo.bitOff;
    newHighMargin = (newHighMargin < 0) ? 0 : newHighMargin;
    const uint bitWidth = getTypeWidth(bitsOp->getOperand(0).getType());
    const uint bitsCutFromLeft = (bitWidth - 1) - hi;
    uint newOffset = linkInfo.off - (bitsCutFromLeft -
        (linkInfo.high - newHighMargin));
    const uint bitsCutFromRight = lo;
    int newOffsetForBits = linkInfo.bitOff - bitsCutFromRight;
    linkInfo.bitOff = newOffsetForBits;
    linkInfo.off = newOffset;
    linkInfo.low = newLowMargin;
    linkInfo.high = newHighMargin;
  } else {
    wireOrBitManValues.pop_back();
  }
}

void LowFIRRTLToNetPass::walkThroughShiftLeft(
    std::vector<Value> &wireOrBitManValues, ShlPrimOp shlOp, LinkInfo &linkInfo,
    std::vector<LinkInfo> &linkInfoCol) {
   wireOrBitManValues.push_back(shlOp->getResult(0));
   linkInfo.bitOff += getTypeWidth(shlOp->getOperand(0).getType());
}

void LowFIRRTLToNetPass::walkThroughPad(std::vector<Value> &wireOrBitManValues,
    PadPrimOp padOp, LinkInfo &linkInfo, std::vector<LinkInfo> &linkInfoCol) {
  wireOrBitManValues.push_back(padOp->getResult(0));
  const uint numBitsPadded = padOp.getAmount() -
      getTypeWidth(padOp->getOperand(0).getType());
  linkInfo.off += numBitsPadded;
}

void LowFIRRTLToNetPass::walkFinal(Operation *op, const uint opNumber,
               const uint typeWidth, LinkInfo &linkInfo,
               std::vector<std::pair<uint,uint>> &fromLinkKeysMargins,
               std::vector<std::vector<LinkKey>> &toLinkKeys) {
  std::vector<LinkKey> toLinkKeysSimple;
  // Reversing the margins because of the difference in endianness.
  const uint lowMarginReversed = typeWidth - linkInfo.high - 1;
  const uint highMarginReversed = typeWidth - linkInfo.low;
  for (uint i = 0; i < highMarginReversed - lowMarginReversed; i++) {
    toLinkKeysSimple.push_back(LinkKey(op, opNumber, linkInfo.off + i));
  }
  toLinkKeys.push_back(toLinkKeysSimple);
  fromLinkKeysMargins.push_back({ lowMarginReversed, highMarginReversed });
}

void LowFIRRTLToNetPass::getToLinkKeysSynthOps(const Value &val,
    FModuleOp fModuleOp, std::vector<std::pair<uint,uint>> &fromLinkKeysMargins,
    std::vector<std::vector<LinkKey>> &toLinkKeys) {
  std::vector<Value> wireOrBitManValues;
  wireOrBitManValues.push_back(val);
  std::vector<LinkInfo> linkInfoCol;
  uint typeWidth = getTypeWidth(val.getType());
  linkInfoCol.push_back({0, typeWidth - 1, 0, 0});
  Operation* prevOp;
  Value prevValue;
  while (!wireOrBitManValues.empty()) {
    Value currentValue = wireOrBitManValues.back();
    wireOrBitManValues.pop_back();
    auto linkInfo = linkInfoCol.back();
    linkInfoCol.pop_back();
    for (auto *user : currentValue.getUsers()) {
      if (auto connect = mlir::dyn_cast<StrictConnectOp>(user)) {
        if (connect.getSrc() == currentValue) {
          Value nextValue = connect.getDest();
          Operation *nextOp = nextValue.getDefiningOp();
          if (nextOp && (isWire(nextOp) || isSimpleLinkMove(nextOp))) {
            wireOrBitManValues.push_back(nextValue);
            linkInfoCol.push_back(linkInfo);
          } else {
            uint opNumber = findOpOperandNumber(nextValue, nextOp, fModuleOp);
            walkFinal(nextOp, opNumber, typeWidth, linkInfo,
                      fromLinkKeysMargins, toLinkKeys);
          }
        }
      } else {
        uint opNumber = findOpOperandNumber(currentValue, user, fModuleOp);
        if (isBitManipulation(user)) {
          if (isConcatenation(user)) {
            auto catOp = mlir::dyn_cast<CatPrimOp>(user);
            walkThroughCat(wireOrBitManValues, {currentValue, catOp},
                          {prevValue, prevOp}, opNumber, linkInfo, linkInfoCol);
          } else if (isBits(user)) {
            auto bitOp = mlir::dyn_cast<BitsPrimOp>(user);
            walkThroughBits(wireOrBitManValues, bitOp, linkInfo, linkInfoCol);
          } else if (isShiftLeft(user)) {
            auto shlOp = mlir::dyn_cast<ShlPrimOp>(user);
            walkThroughShiftLeft(wireOrBitManValues, shlOp, linkInfo,
                                 linkInfoCol);
          } else if (isPad(user)) {
            auto padOp = mlir::dyn_cast<PadPrimOp>(user);
            walkThroughPad(wireOrBitManValues, padOp, linkInfo, linkInfoCol);
          } else if (isSimpleLinkMove(user)) {
            wireOrBitManValues.push_back(user->getResult(0));
          }
          linkInfoCol.push_back(linkInfo);
        } else {
          walkFinal(user, opNumber, typeWidth, linkInfo,
                    fromLinkKeysMargins, toLinkKeys);
        }
      }
      prevOp = user;
      prevValue = currentValue;
    }
  }
}

void LowFIRRTLToNetPass::processInstance(InstanceOp instOp,
    NetBuilder *netBuilder) {
  const uint bitWidthIn = getBitWidthIn(instOp);
  const uint bitWidthOut = getBitWidthOut(instOp);
  const CellSymbol cellSymbol = getCellSymbol(instOp);
  const std::string &cellTypeName = instOp.getModuleName().str();
  CellTypeKey cellTypeKey(cellTypeName, bitWidthIn, bitWidthOut);
  CellTypeID cellTypeID;
  if (cellTypeKeyToCellTypeIDs.count(cellTypeKey) != 0) {
    cellTypeID = cellTypeKeyToCellTypeIDs[cellTypeKey];
  } else {
    cellTypeID = makeSoftType(cellSymbol,
                              cellTypeName,
                              model::OBJ_NULL_ID,
                              bitWidthIn,
                              bitWidthOut);
  }
  std::vector<LinkEnd> linkEnds;
  for (uint i = 0; i < bitWidthIn; i++) {
    linkEnds.push_back(LinkEnd());
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
  const uint bitWidthIn = getBitWidthIn(synthOp);
  const uint bitWidthOut = getBitWidthOut(synthOp);
  const CellSymbol cellSymbol = getCellSymbol(synthOp);
  auto linkEnds = getLinkEnds(synthOp, fModuleOp);
  const std::string &cellTypeName = synthOp->getName().stripDialect().str();
  CellTypeKey cellTypeKey(cellTypeName, bitWidthIn, bitWidthOut);
  CellTypeID cellTypeID;
  if (cellTypeKeyToCellTypeIDs.count(cellTypeKey) != 0) {
    cellTypeID = cellTypeKeyToCellTypeIDs[cellTypeKey];
  } else {
    cellTypeID = makeSoftType(cellSymbol,
                              cellTypeName,
                              model::OBJ_NULL_ID,
                              bitWidthIn,
                              bitWidthOut);
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
    const uint bitWidthIn,
    std::vector<LinkEnd> &linkEnds, std::vector<LinkEnd> &outLinkEnds,
    NetBuilder *netBuilder) {
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
  const uint lowMarginReversed = bitWidthIn - highMargin - 1;
  const uint highMarginReversed = bitWidthIn - lowMargin;

  for (uint i = lowMarginReversed; i < highMarginReversed; i++) {
    outLinkEnds.push_back(linkEnds[i]);
  }
}

void LowFIRRTLToNetPass::processBitManipulation(Operation *op, 
    FModuleOp fModuleOp, NetBuilder *netBuilder, CellID &cellIDForZero) {
  auto linkEnds = getLinkEnds(op, fModuleOp);
  std::vector<LinkEnd> outLinkEnds;
  const uint bitWidthIn = getBitWidthIn(op);
  if (isPad(op)) {
    auto padOp = mlir::dyn_cast<PadPrimOp>(op);
    processPad(padOp, cellIDForZero, bitWidthIn, linkEnds, outLinkEnds,
               netBuilder);
  } else if (isShiftLeft(op)) {
    auto shlOp = mlir::dyn_cast<ShlPrimOp>(op);
    processShiftLeft(shlOp, cellIDForZero, bitWidthIn, linkEnds, outLinkEnds,
                     netBuilder);
  } else if (isShiftRight(op)) {
    auto shrOp = mlir::dyn_cast<ShrPrimOp>(op);
    processShiftRight(shrOp, cellIDForZero, bitWidthIn, linkEnds, outLinkEnds,
                      netBuilder);
  } else if (isConcatenation(op) || isSimpleLinkMove(op)) {
    for (uint i = 0; i < bitWidthIn; i++) {
      outLinkEnds.push_back(linkEnds[i]);
    }
  } else if (isHead(op)) {
    auto headOp = mlir::dyn_cast<HeadPrimOp>(op);
    processHead(headOp, linkEnds, outLinkEnds);
  } else if (isTail(op)) {
    auto tailOp = mlir::dyn_cast<TailPrimOp>(op);
    processTail(tailOp, bitWidthIn, linkEnds, outLinkEnds);
  } else if (isBits(op)) {
    auto bitsOp = mlir::dyn_cast<BitsPrimOp>(op);
    processBits(bitsOp, bitWidthIn, linkEnds, outLinkEnds);
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

void LowFIRRTLToNetPass::processBoolLogic(Operation *boolLogiOp,
    FModuleOp fModuleOp, NetBuilder *netBuilder) {
  auto linkEnds = getLinkEnds(boolLogiOp, fModuleOp);
  const CellSymbol cellSymbol = getCellSymbol(boolLogiOp);
  const CellTypeID cellTypeID = getCellTypeID(cellSymbol);
  const uint dataWidth = getTypeWidth(boolLogiOp->getResult(0).getType());
  const uint inCount = getInCount(boolLogiOp);
  for (uint j = 0; j < dataWidth; j++) {
    std::vector<LinkEnd> linkEndsForOne;
    for (uint i = 0; i < inCount; i++) {
      linkEndsForOne.push_back(linkEnds[i * dataWidth + j]);
    }
    CellID cellDestID = makeCell(cellTypeID, linkEndsForOne);
    netBuilder->addCell(cellDestID);
    const LinkKey destKey(boolLogiOp, 0, j);
    linkKeyToLinkEndOuts.emplace(std::make_pair(destKey, LinkEnd(cellDestID)));
    const LinkKey firstArgKey(boolLogiOp, 0, j);
    std::vector<CellID> cellIDs0;
    cellIDs0.push_back(cellDestID);
    cellKeyToCellIDsIns.emplace(std::make_pair(firstArgKey, cellIDs0));
    const LinkKey secondArgKey(boolLogiOp, 1, j);
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
  const uint dataWidth = getTypeWidth(boolLogicRop->getOperand(0).getType());
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
  const uint dataWidth = getTypeWidth(regOp->getResult(0).getType());
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
  const uint dataWidth = getTypeWidth(regResetOp->getResult(0).getType());
  const uint resetValueWidth =
      getTypeWidth(regResetOp->getOperand(2).getType());
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
      uassert(false, "Invalid operation in 'LoFIRRTL' code:" << destOpName <<
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
    // For debug purposes.
#ifdef UTOPIA_DEBUG
    checkConnections(cellKeyToCellIDsIns);
#endif
    NetID netID = netBuilder.make();
    const std::string &cellName = fModuleOp.getName().str();
    CellTypeID cellTypeID = makeSoftType(CellSymbol::UNDEF,
                                         cellName,
                                         netID,
                                         Net::get(netID).getInNum(),
                                         Net::get(netID).getOutNum());
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

#ifdef UTOPIA_DEBUG
void LowFIRRTLToNetPass::checkConnections() {
  // Check whether all cells have been correctly connected.
  uint count = 0;
  for (const auto &[cellKey, cellIDIns] : cellKeyToCellIDsIns) {
    Operation* op = cellKey.op;
    const uint portNum = cellKey.portNum;
    const uint bitNum = cellKey.bitNum;
    // For wires keys with empty dummies were created.
    if (op && isWire(op)) {
      continue;
    }
    std::cout << "Cell key:\n";
    std::cout << op << " " << portNum << " " << bitNum << "\n";
    if (op) {
      std::cout << op->getName().getIdentifier().str() << "\n";
    }
    std::cout << "Cell IDs:\n";
    for (const auto &cellID : cellIDIns) {
      std::cout << cellID << "\n";
      const auto& cell = Cell::get(cellID);
      for (uint i = 0; i < cell.getFanin(); i++) {
        if (!(cell.getLink(i)).isValid()) {
          count++;
          std::cout << count << "\n";
          std::cout << "Link " << i << " remains unconnected!" << "\n";
        }
      }
    }
  }
}
#endif

} // namespace

std::unique_ptr<Pass> createCHIRRTLToLowFIRRTLPass() {
  return std::make_unique<CHIRRTLToLowFIRRTLPass>();
}

std::unique_ptr<Pass> createLowFIRRTLToNetPass(
    std::shared_ptr<std::vector<CellTypeID>> resultNetlist) {
  return std::make_unique<LowFIRRTLToNetPass>(resultNetlist);
}