//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/analyzer/probabilistic_estimate.h"
#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/fraig_checker.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/debugger/sat_checker.h"
#include "gate/estimator/ppa_estimator.h"
#include "gate/library/liberty_manager.h"
#include "gate/model/design.h"
#include "gate/model/net.h"
#include "gate/model/printer/printer.h"
#include "gate/optimizer/get_dbstat.h"
#include "gate/optimizer/pass.h"
#include "gate/techmapper/techmapper.h"
#include "gate/translator/firrtl.h"
#include "gate/translator/graphml.h"
#include "gate/translator/yosys_converter_firrtl.h"
#include "gate/translator/yosys_converter_model2.h"
#include "util/env.h"

#include <CLI/CLI.hpp>
#include <tcl.h>

#define PARAM_SUBCOMMAND(app, cmd, func) do {\
  processSubcommand(app, #cmd, [&](){ measureAndRun(#cmd, func); });\
} while (false)

#define SUBCOMMAND(cli, cmd) do {\
  processSubcommand((cli), #cmd, [&]() {\
    measureAndRun(#cmd, [&](){\
        foreach(pass::cmd())->transform(designBuilder); });\
  });\
} while (false)

using namespace eda::gate::model;
using namespace eda::gate::optimizer;

static DesignBuilderPtr designBuilder = nullptr;
static std::string previousStep = "none";
static bool measureTime = false;

static int printFile(Tcl_Interp *interp, const std::string &fileName) {
  const char *utopiaHome = std::getenv("UTOPIA_HOME");
  if (!utopiaHome) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("UTOPIA_HOME is not set", -1));
    return TCL_ERROR;
  }

  std::string filePath = std::string(utopiaHome) + "/" + fileName;
  std::ifstream file(filePath);

  if (!file.is_open()) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("Unable to open file", -1));
    return TCL_ERROR;
  }

  std::string line;
  while (getline(file, line)) {
    std::cout << line << std::endl;
  }

  file.close();
  return TCL_OK;
}

static int printTitle(Tcl_Interp *interp) {
  return printFile(interp, "doc/Title.md");
}

static int readVerilog(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {

  if (designBuilder != nullptr) {
    Tcl_SetObjResult(
        interp,
        Tcl_NewStringObj("The design has already been uploaded", -1));
    return TCL_ERROR;
  }

  CLI::App app;

  std::string frontend = "yosys";
  std::string topModule;
  bool debugMode = false;

  app.add_option("--frontend", frontend);
  app.add_option("--top", topModule);
  app.add_flag("--debug", debugMode);
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  if (app.remaining().empty()) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("No file specified", -1));
    return TCL_ERROR;
  }

  if (!std::filesystem::exists(app.remaining().at(0))) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("File doesn't exist", -1));
    return TCL_ERROR;
  }

  if (frontend == "yosys") {
    YosysToModel2Config cfg;
    cfg.debugMode = debugMode;
    cfg.topModule = topModule;
    cfg.files = app.remaining();

    YosysConverterModel2 cvt(cfg);
    designBuilder = std::make_unique<DesignBuilder>(cvt.getNetID());

    designBuilder->save("original");
    return TCL_OK;
  }

  Tcl_SetObjResult(interp, Tcl_NewStringObj("Unsupported frontend", -1));
  return TCL_ERROR;
}

void printModel(
    const std::string &fileName,
    ModelPrinter::Format format,
    const Net &net) {

  ModelPrinter &printer = ModelPrinter::getPrinter(format);

  if (!fileName.empty()) {
    std::ofstream outFile(fileName);
    printer.print(outFile, net, "printedNet");
    outFile.close();
  } else {
    printer.print(std::cout, net);
  }
}

static int writeDesign(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {

  if (designBuilder == nullptr) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("Design is not loaded", -1));
    return TCL_ERROR;
  }

  CLI::App app;

  std::string fileName = "design.v";
  ModelPrinter::Format format = ModelPrinter::VERILOG;

  const std::map<std::string, ModelPrinter::Format> formatMap {
    {"verilog", ModelPrinter::VERILOG},
    {"simple", ModelPrinter::SIMPLE},
    {"dot", ModelPrinter::DOT},
  };

  app.add_option("--format", format, "Output format")
      ->expected(1)
      ->transform(CLI::CheckedTransformer(formatMap, CLI::ignore_case));
  app.add_option("--path", fileName);
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  if (format == ModelPrinter::VERILOG ||
      format == ModelPrinter::SIMPLE ||
      format == ModelPrinter::DOT) {
    const auto &net = Net::get(designBuilder->make());
    printModel(fileName, format, net);
    return TCL_OK;
  }

  Tcl_SetObjResult(interp, Tcl_NewStringObj("Non-existent format", -1));
  return TCL_ERROR;
}

template <typename CheckerType>
bool checkEquivalence() {
  CheckerType &checker = CheckerType::get();
  return checker.areEquivalent(*designBuilder, previousStep, "original").equal();
}

bool containsFalse(const std::vector<bool> &eq) {
  return std::find(eq.begin(), eq.end(), false) != eq.end();
}

static int lec(ClientData, Tcl_Interp *interp, int argc, const char *argv[]) {

  using eda::gate::debugger::BddChecker;
  using eda::gate::debugger::FraigChecker;
  using eda::gate::debugger::options::LecType;
  using eda::gate::debugger::RndChecker;
  using eda::gate::debugger::SatChecker;

  if (designBuilder == nullptr) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("Design is not loaded", -1));
    return TCL_ERROR;
  }

  if (previousStep == "none") {
    Tcl_SetObjResult(
        interp,
        Tcl_NewStringObj("There is nothing to compare with", -1));
    return TCL_ERROR;
  }

  CLI::App app;

  LecType method = LecType::SAT;

  const std::map<std::string, LecType> lecMethodMap {
    {"bdd", LecType::BDD},
    {"fra", LecType::FRAIG},
    {"rnd", LecType::RND},
    {"sat", LecType::SAT},
  };

  app.add_option("--method", method, "Method for checking equivalence")
      ->expected(1)
      ->transform(CLI::CheckedTransformer(lecMethodMap, CLI::ignore_case));
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  std::vector<bool> eq;

  switch (method) {
    case LecType::SAT:
      eq.push_back(checkEquivalence<SatChecker>());
      break;
    case LecType::BDD:
      eq.push_back(checkEquivalence<BddChecker>());
      break;
    case LecType::RND:
      eq.push_back(checkEquivalence<RndChecker>());
      break;
    case LecType::FRAIG:
      eq.push_back(checkEquivalence<FraigChecker>());
      break;
    default:
      Tcl_SetObjResult(interp, Tcl_NewStringObj("Non-existent checker", -1));
      return TCL_ERROR;
  }

  const char *result = containsFalse(eq) ? "false" : "true";
  Tcl_SetObjResult(interp, Tcl_NewStringObj(result, -1));

  return TCL_OK;
}

static int techMap(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {

  using eda::gate::library::LibertyManager;
  using eda::gate::techmapper::Techmapper;

  if (designBuilder == nullptr) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("Design is not loaded", -1));
    return TCL_ERROR;
  }

  if (LibertyManager::get().getLibraryName().empty()) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("No path to liberty", -1));
    return TCL_ERROR;
  }

  CLI::App app;

  const std::map<std::string, Techmapper::Strategy> mapperTypeMap {
    {"af", Techmapper::Strategy::AREA_FLOW},
    {"area", Techmapper::Strategy::AREA},
    {"delay", Techmapper::Strategy::DELAY},
    {"power", Techmapper::Strategy::POWER},
  };

  auto mapperType = Techmapper::Strategy::AREA;

  app.add_option("--type", mapperType, "Type of mapper")
      ->expected(1)
      ->transform(CLI::CheckedTransformer(mapperTypeMap, CLI::ignore_case));
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  const auto &techLib = LibertyManager::get().getLibraryName();

  const std::filesystem::path sdcPath = eda::env::getHomePath();

  Techmapper techmapper;
  techmapper.setStrategy(mapperType);
  techmapper.setSDC(sdcPath);
  techmapper.setLibrary(techLib);

  const size_t numSubnets = designBuilder->getSubnetNum();
  for (size_t subnetId = 0; subnetId < numSubnets; ++subnetId) {
    const auto &subnetBuilder = designBuilder->getSubnetBuilder(subnetId);

    eda::gate::premapper::AigMapper aigMapper("aig");
    const auto premappedSubnetID = aigMapper.transform(subnetBuilder->make());

    SubnetBuilder subnetBuilderTechmap;
    techmapper.techmap(premappedSubnetID, subnetBuilderTechmap);
    const auto mappedSubnetID = subnetBuilderTechmap.make();

    designBuilder->setSubnetBuilder(
        subnetId,
        std::make_shared<SubnetBuilder>(mappedSubnetID));
  }

  designBuilder->save("techmap");
  previousStep = "techmap";

  return TCL_OK;
}

static int readLiberty(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {

  using eda::gate::library::LibertyManager;

  CLI::App app;

  std::string path = "";
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  if (!app.remaining().empty()) {
    path = app.remaining().at(0);
  } else {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("No path to liberty", -1));
    return TCL_ERROR;
  }

  if (!std::filesystem::exists(path)){
    Tcl_SetObjResult(interp, Tcl_NewStringObj("File doesn't exist", -1));
    return TCL_ERROR;
  }

  LibertyManager::get().loadLibrary(path);
  return TCL_OK;
}

void printSubnet(
    const DesignBuilderPtr &designBuilder,
    size_t subnetId) {

  const auto &subnetBuilder = designBuilder->getSubnetBuilder(subnetId);
  const auto &subnet = Subnet::get(subnetBuilder->make(true));
  std::cout << subnet << '\n';
}

static int writeSubnet(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {

  if (designBuilder == nullptr) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("Design is not loaded", -1));
    return TCL_ERROR;
  }

  CLI::App app;

  size_t number = 0;
  auto *entered = app.add_option("--index", number, "Subnet sequence number");
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  const size_t numSubnets = designBuilder->getSubnetNum();
  if (entered->count() == 0) {
    for (size_t subnetId = 0; subnetId < numSubnets; ++subnetId) {
      printSubnet(designBuilder, subnetId);
    }
  } else if (number < numSubnets) {
    printSubnet(designBuilder, number);
  } else {
    Tcl_SetObjResult(
        interp,
        Tcl_NewStringObj("There is no such subnet nubmer", -1));
    return TCL_ERROR;
  }

  return TCL_OK;
}

static int readGraphMl(
    ClientData,Tcl_Interp *interp,
    int argc,
    const char *argv[]) {

  using eda::gate::translator::GmlTranslator;

  if (designBuilder != nullptr) {
    Tcl_SetObjResult(
        interp,
        Tcl_NewStringObj("The design has already been uploaded", -1));
    return TCL_ERROR;
  }

  CLI::App app;

  std::string fileName = "";
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  if (!app.remaining().empty()) {
    fileName = app.remaining().at(0);
  } else {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("No file specified", -1));
    return TCL_ERROR;
  }

  if (!std::filesystem::exists(fileName)){
    Tcl_SetObjResult(interp, Tcl_NewStringObj("File doesn't exist", -1));
    return TCL_ERROR;
  }

  if (!fileName.empty()) {
    GmlTranslator::ParserData data;
    GmlTranslator parser;
    const auto &subnet = parser.translate(fileName, data)->make(true);
    designBuilder = std::make_unique<DesignBuilder>(subnet);
    designBuilder->save("original");
    return TCL_OK;
  }

  Tcl_SetObjResult(interp, Tcl_NewStringObj("File doesn't exist", -1));
  return TCL_ERROR;
}

static int stats(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {

  namespace estimator = eda::gate::estimator;

  if (designBuilder == nullptr) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("Design is not loaded", -1));
    return TCL_ERROR;
  }

  CLI::App app;

  bool logic = false;
  app.add_flag("-l, --logical", logic, "Logic level characteristics");
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  for (size_t i = 0; i < designBuilder->getSubnetNum(); ++i) {
    const auto &id = designBuilder->getSubnetID(0);
    const auto &subnet = Subnet::get(id);
    if (logic) {
      eda::gate::analyzer::ProbabilityEstimator estimator;
      std::cout << "Area: " <<
          subnet.getEntries().size() << '\n';
      std::cout << "Delay: " <<
          subnet.getPathLength().second << '\n';
      /// FIXME: Use SubnetBuilder instead of Subnet
      SubnetBuilder builder(subnet);
      std::cout << "Power: " <<
          estimator.estimate(builder).getSwitchProbsSum() << '\n';
    } else {
      if (previousStep != "techmap") {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj(
                "Physical properties are not available without a techmap",
                -1));
        return TCL_ERROR;
      }
      std::cout << "Area: " << estimator::getArea(id) << '\n';
      std::cout << "Delay: " << estimator::getArrivalTime(id) << '\n';
      std::cout << "Power: " << estimator::getLeakagePower(id) << '\n';
    }
  }

  return TCL_OK;
}

static int clear(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  designBuilder = nullptr;
  return TCL_OK;
}

template<typename T>
void processSubcommand(
    CLI::App &app,
    const std::string &subcommandName,
    T handler) {

  if (app.got_subcommand(subcommandName)) {
    handler();
  }
}

template<typename Func>
void measureAndRun(const std::string &name, Func func) {
  if (measureTime) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << name << " took " << elapsed.count() << " seconds\n";
  } else {
    func();
  }
}

static int pass(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {

  namespace pass = eda::gate::optimizer;

  if (designBuilder == nullptr) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("Design is not loaded", -1));
    return TCL_ERROR;
  }

  CLI::App app;

  // Standart parametres for specific passes
  // Rewriter
  std::string rwName  = "rw";
  uint16_t rwK        = 4;
  bool rwZ            = false;
  // Resubstiture
  std::string rsName  = "rs";
  uint16_t rsK        = 8;
  uint16_t rsN        = 16;
  // Resubstiture w/ zero-cost replacements
  std::string rszName = "rsz";
  uint16_t rszK       = 8;
  uint16_t rszN       = 16;

  app.add_subcommand("aig", "Mapping to the AIG representation");
  app.add_subcommand("mig", "Mapping to the MIG represenation");
  app.add_subcommand("b", "Depth-aware balancing");

  auto *passRw = app.add_subcommand("rw", "Rewriting");
  passRw->add_option("--name", rwName);
  passRw->add_option("-k", rwK);
  passRw->add_flag("-z", rwZ);

  app.add_subcommand("rwz", "Rewrite zero-cost replacements");
  app.add_subcommand("rf", "Refactor");
  app.add_subcommand("rfz", "Refactor zero-cost replacements");
  app.add_subcommand("rfa", "Refactor criterion area");
  app.add_subcommand("rfd", "Refactor criterion delay");
  app.add_subcommand("rfp", "Refactor criterion power");

  auto *passRs = app.add_subcommand("rs", "Resubstitute");
  passRs->add_option("--name", rsName);
  passRs->add_option("-k", rsK);
  passRs->add_option("-n", rsN);

  auto *passRsz = app.add_subcommand(
      "rsz",
      "Resubstitute w/ zero-cost replacements");
  passRsz->add_option("--name", rszName);
  passRsz->add_option("-k", rszK);
  passRsz->add_option("-n", rszN);

  app.add_subcommand("ma", "Technology Mapper criterion area");
  app.add_subcommand("md", "Technology Mapper criterion delay");
  app.add_subcommand("mp", "Technology Mapper criterion power");
  app.add_subcommand("resyn", "Pre-defined script resyn");
  app.add_subcommand("resyn2", "Pre-defined script resyn2");
  app.add_subcommand("resyn2a", "Pre-defined script resyn2a");
  app.add_subcommand("resyn3", "Pre-defined script resyn3");
  app.add_subcommand("compress", "Pre-defined script compress");
  app.add_subcommand("compress2", "Pre-defined script compress2");
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  SUBCOMMAND(app, aig);
  SUBCOMMAND(app, mig);
  SUBCOMMAND(app, b);

  PARAM_SUBCOMMAND(app, rw, [&]() {
    foreach(pass::rw(rwName, rwK, rwZ))->transform(designBuilder);
  });

  SUBCOMMAND(app, rwz);
  SUBCOMMAND(app, rf);
  SUBCOMMAND(app, rfz);
  SUBCOMMAND(app, rfa);
  SUBCOMMAND(app, rfd);
  SUBCOMMAND(app, rfp);

  PARAM_SUBCOMMAND(app, rs, [&]() {
    foreach(pass::rs(rsName, rsK, rsN))->transform(designBuilder);
  });

  PARAM_SUBCOMMAND(app, rsz, [&]() {
    foreach(pass::rsz(rszName, rszK, rszN))->transform(designBuilder);
  });

  SUBCOMMAND(app, resyn);
  SUBCOMMAND(app, resyn2);
  SUBCOMMAND(app, resyn2a);
  SUBCOMMAND(app, resyn3);
  SUBCOMMAND(app, compress);
  SUBCOMMAND(app, compress2);
  SUBCOMMAND(app, ma);
  SUBCOMMAND(app, md);
  SUBCOMMAND(app, mp);

  designBuilder->save("pass");
  previousStep = "pass";

  return TCL_OK;
}

static int showTime(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  measureTime = !measureTime;
  return TCL_OK;
}

static int help(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return printFile(interp, "doc/CLI.md");
}

static int verilogToFir(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {

  CLI::App app;

  std::string outputFile = "out.fir";
  std::string topModule;
  bool debugMode = false;

  app.add_flag("--debug", debugMode);
  app.add_option("--top", topModule);
  app.add_option("-o, --out", outputFile);
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  if (app.remaining().empty()) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("No files specified", -1));
    return TCL_ERROR;
  }

  for (const auto &file: app.remaining()) {
    if (!std::filesystem::exists(app.remaining().at(0))) {
      std::cout << file;
      Tcl_SetObjResult(interp, Tcl_NewStringObj(" doesn't exist", -1));
      return TCL_ERROR;
    }
  }

  FirrtlConfig cfg;
  cfg.debugMode = debugMode;
  cfg.outputFileName = outputFile;
  cfg.topModule = topModule;
  cfg.files = app.remaining();
  YosysConverterFirrtl converter(cfg);

  return TCL_OK;
}

static int dbStat(ClientData, Tcl_Interp *interp, int argc,
                  const char *argv[]) {

  CLI::App app;

  std::string dbPath;
  int ttSize;
  std::string outputType = "BOTH";
  std::string outputNamefile;

  app.add_option("--db", dbPath)->expected(1)->required(true);
  app.add_option("--otype", outputType)->expected(1);
  app.add_option("--out", outputNamefile)->expected(1);
  app.add_option("--ttsize", ttSize)->expected(1)->required(true);

  /// Input output value(s).
  app.allow_extras();

  try {
    app.parse(argc, argv);
  } catch (CLI::ParseError &e) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
    return TCL_ERROR;
  }

  if (app.remaining().empty()) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("No files specified", -1));
    return TCL_ERROR;
  }

  eda::gate::optimizer::NPNDBConfig config;
  config.dbPath = dbPath;

  if (outputType == "DOT") {
    config.outType = eda::gate::optimizer::OutType::DOT;
  } else if (outputType == "INFO") {
    config.outType = eda::gate::optimizer::OutType::INFO;
  } else if (outputType == "BOTH") {
    config.outType = eda::gate::optimizer::OutType::BOTH;
  } else {
    std::cerr << "Wrong type of output: " << outputType
              << ", correct are (DOT / INFO / BOTH)" << std::endl;
    return TCL_ERROR;
  }

  config.outName = outputNamefile;
  config.ttSize = ttSize;
  config.binLines = app.remaining();

  if (eda::gate::optimizer::getDbStat(std::cerr, config)) {
    return TCL_ERROR;
  }

  return TCL_OK;
}

int Utopia_TclInit(Tcl_Interp *interp) {
  if ((Tcl_Init)(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  printTitle(interp);

  Tcl_DeleteCommand(interp, "exec");
  Tcl_DeleteCommand(interp, "unknown");

  Tcl_CreateCommand(interp, "clear",          clear,        nullptr, nullptr);
  Tcl_CreateCommand(interp, "dbstat",         dbStat,       nullptr, nullptr);
  Tcl_CreateCommand(interp, "help",           help,         nullptr, nullptr);
  Tcl_CreateCommand(interp, "lec",            lec,          nullptr, nullptr);
  Tcl_CreateCommand(interp, "pass",           pass,         nullptr, nullptr);
  Tcl_CreateCommand(interp, "read_graphml",   readGraphMl,  nullptr, nullptr);
  Tcl_CreateCommand(interp, "read_liberty",   readLiberty,  nullptr, nullptr);
  Tcl_CreateCommand(interp, "read_verilog",   readVerilog,  nullptr, nullptr);
  Tcl_CreateCommand(interp, "show_time",      showTime,     nullptr, nullptr);
  Tcl_CreateCommand(interp, "stats",          stats,        nullptr, nullptr);
  Tcl_CreateCommand(interp, "techmap",        techMap,      nullptr, nullptr);
  Tcl_CreateCommand(interp, "verilog_to_fir", verilogToFir, nullptr, nullptr);
  Tcl_CreateCommand(interp, "write_design",   writeDesign,  nullptr, nullptr);
  Tcl_CreateCommand(interp, "write_subnet",   writeSubnet,  nullptr, nullptr);

  return TCL_OK;
}
