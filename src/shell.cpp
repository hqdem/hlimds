//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "config.h"
#include "gate/analyzer/probabilistic_estimate.h"
#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/fraig_checker.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/debugger/sat_checker.h"
#include "gate/estimator/ppa_estimator.h"
#include "gate/library/library_parser.h"
#include "gate/model/design.h"
#include "gate/model/net.h"
#include "gate/model/printer/printer.h"
#include "gate/optimizer/get_dbstat.h"
#include "gate/optimizer/pass.h"
#include "gate/techmapper/techmapper_wrapper.h"
#include "gate/translator/firrtl.h"
#include "gate/translator/graphml.h"
#include "gate/translator/yosys_converter_firrtl.h"
#include "gate/translator/yosys_converter_model2.h"
#include "util/env.h"

#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <tcl.h>

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define UTOPIA_OUT std::cout

using namespace eda::gate::model;
using namespace eda::gate::debugger;
using namespace eda::gate::debugger::options;
using namespace eda::gate::library;
using namespace eda::gate::optimizer;
using namespace eda::gate::techmapper;
using namespace eda::gate::translator;

//===----------------------------------------------------------------------===//
// Utility Functions
//===----------------------------------------------------------------------===//

template <typename Clock>
static inline void printTime(const std::string &name,
                             const std::chrono::time_point<Clock> &start,
                             const std::chrono::time_point<Clock> &end,
                             const std::string &prefix = "") {
  std::chrono::duration<double> elapsed = end - start;
  UTOPIA_OUT << prefix << name << ": "
             << std::fixed << elapsed.count() << "s"
             << std::endl << std::flush;
}

static inline int makeResult(Tcl_Interp *interp, const std::string &result) {
  Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
  return TCL_OK;
}

static inline int makeError(Tcl_Interp *interp, const std::string &error) {
  makeResult(interp, fmt::format("error: {}", error));
  return TCL_ERROR;
} 

//===----------------------------------------------------------------------===//
// Base Classes
//===----------------------------------------------------------------------===//

/**
 * @brief Utopia EDA shell command interface.
 */
struct UtopiaCommand {
  UtopiaCommand(const char *name, const char *desc):
    name(name), desc(desc), app(desc, name) {
    // CLI::App adds the help option, but it is not required.
    auto *helpOption = app.get_help_ptr();
    if (helpOption) {
      app.remove_option(helpOption);
    }
  }

  virtual int run(Tcl_Interp *interp, int argc, const char *argv[]) = 0;

  virtual int runEx(Tcl_Interp *interp, int argc, const char *argv[]) {
    using clock = std::chrono::high_resolution_clock;

    auto start = clock::now();
    int status = run(interp, argc, argv);
    auto end = clock::now();

    printTime<clock>(fmt::format("{}({})", name, status), start, end, "> ");
    return status;
  }

  void printHelp(std::ostream &out) const {
    out << app.help() << std::flush;
  }

  const char *name;
  const char *desc;

  CLI::App app;
};

/**
 * @brief Utopia EDA shell command registry.
 */
class UtopiaCommandRegistry final {
public:
  void addCommand(UtopiaCommand *command) {
    assert(command);
    commands.emplace(std::string(command->name), command);
  }

  UtopiaCommand *getCommand(const std::string &name) {
    auto i = commands.find(name);
    return i != commands.end() ? i->second : nullptr;
  }

  void printHelp(std::ostream &out) const {
    for (const auto &[name, command] : commands) {
      out << std::string(2, ' ')
          << std::setw(20)
          << std::left
          << name
          << command->desc
          << std::endl;
    }

    out << std::endl;
    out << "Type 'help <command>' for more information on a command.";
    out << std::endl << std::flush;
  }

private:
  std::map<std::string, UtopiaCommand*> commands;
};

static UtopiaCommandRegistry commandRegistry;
static DesignBuilderPtr designBuilder = nullptr;
static std::string previousStep = "none";

//===----------------------------------------------------------------------===//
// Command: Database Statistics
//===----------------------------------------------------------------------===//

struct DbStatCommand final : public UtopiaCommand {
  DbStatCommand():
      UtopiaCommand("dbstat", "Prints information about a subnet database") {
    app.add_option("--db", dbPath)->expected(1)->required(true);
    app.add_option("--otype", outputType)->expected(1);
    app.add_option("--out", outputNamefile)->expected(1);
    app.add_option("--ttsize", ttSize)->expected(1)->required(true);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    if (app.remaining().empty()) {
      return makeError(interp, "no input files");
    }

    NPNDBConfig config;
    config.dbPath = dbPath;

    if (outputType == "DOT") {
      config.outType = OutType::DOT;
    } else if (outputType == "INFO") {
      config.outType = OutType::INFO;
    } else if (outputType == "BOTH") {
      config.outType = OutType::BOTH;
    } else {
      return makeError(interp,
          fmt::format("unknown output type '{}'", outputType));
    }

    config.outName = outputNamefile;
    config.ttSize = ttSize;
    config.binLines = app.remaining();

    return getDbStat(UTOPIA_OUT, config) ? TCL_ERROR : TCL_OK;
  }

  std::string dbPath;
  int ttSize;
  std::string outputType = "BOTH";
  std::string outputNamefile;
};

static DbStatCommand dbstatCmd;

static int CmdDbStat(
    ClientData,
    Tcl_Interp *interp, int argc,
    const char *argv[]) {
  return dbstatCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Delete
//===----------------------------------------------------------------------===//

struct DeleteCommand final : public UtopiaCommand {
  DeleteCommand():
      UtopiaCommand("delete", "Erases the design from memory") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    designBuilder = nullptr;
    return TCL_OK;
  }
};

static DeleteCommand deleteCmd;

static int CmdDelete(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return deleteCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Exit
//===----------------------------------------------------------------------===//

struct ExitCommand final : public UtopiaCommand {
  ExitCommand():
      UtopiaCommand("exit", "Closes the interactive shell") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    return TCL_OK;
  }
};

static ExitCommand exitCmd;

// Default implementation.

//===----------------------------------------------------------------------===//
// Command: Help
//===----------------------------------------------------------------------===//

struct HelpCommand final : public UtopiaCommand {
  HelpCommand():
      UtopiaCommand("help", "Prints help information") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    if (app.remaining().empty()) {
      commandRegistry.printHelp(UTOPIA_OUT);
      return TCL_OK;
    }

    auto name = app.remaining().at(0);
    auto *command = commandRegistry.getCommand(name);

    if (command) {
      command->printHelp(UTOPIA_OUT);
      return TCL_OK;
    }

    return makeError(interp, fmt::format("unknown command '{}'", name));
  }
};

static HelpCommand helpCmd;

static int CmdHelp(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return helpCmd.run/* simple */(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: LEC
//===----------------------------------------------------------------------===//

template <typename CheckerType>
static bool checkEquivalence() {
  auto &checker = CheckerType::get();
  return checker.areEquivalent(*designBuilder, previousStep, "original").equal();
}

static bool containsFalse(const std::vector<bool> &eq) {
  return std::find(eq.begin(), eq.end(), false) != eq.end();
}

struct LecCommand final : public UtopiaCommand {
  LecCommand():
      UtopiaCommand("lec", "Checks logical equivalence") {
    const std::map<std::string, LecType> lecMethodMap {
      { "bdd", LecType::BDD   },
      { "fra", LecType::FRAIG },
      { "rnd", LecType::RND   },
      { "sat", LecType::SAT   }
    };

    app.add_option("--method", method, "Method for checking equivalence")
        ->expected(1)
        ->transform(CLI::CheckedTransformer(lecMethodMap, CLI::ignore_case));
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (!designBuilder) {
      return makeError(interp, "design has not been loaded");
    }

    if (previousStep == "none") {
      return makeError(interp, "no checkpoint to compare with");
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
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
        return makeError(interp, "unknown checker");
    }

    const char *result = containsFalse(eq) ? "false" : "true";
    return makeResult(interp, result);
  }

  LecType method = LecType::SAT;
};

static LecCommand lecCmd;

static int CmdLec(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return lecCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Logical Optimization
//===----------------------------------------------------------------------===//

#define ADD_CUSTOM_CMD(app, name, desc, callback)\
  app.add_subcommand(name, desc)->parse_complete_callback([&]() {\
    measureAndRun(name, callback);\
  })

#define ADD_CMD(app, cmd, name, desc)\
  ADD_CUSTOM_CMD(app, name, desc, [&]() {\
    foreach(cmd())->transform(designBuilder);\
  })

template<typename Func>
static void measureAndRun(const std::string &name, Func func) {
  using clock = std::chrono::high_resolution_clock;

  auto start = clock::now();
  func();
  auto end = clock::now();

  printTime<clock>(name, start, end, "  - ");
}

struct LogOptCommand final : public UtopiaCommand {
  LogOptCommand() :
      UtopiaCommand("logopt", "Applies an optimization pass to the design") {
    namespace pass = eda::gate::optimizer;

    // Premapping.
    ADD_CMD(app, pass::aig, "aig", "Mapping to AIG");
    ADD_CMD(app, pass::mig, "mig", "Mapping to MIG");

    // Balancing.
    ADD_CMD(app, pass::b, "b", "Depth-aware balancing");

    // Rewriting.
    auto *passRw = ADD_CUSTOM_CMD(app,
        "rw", "Rewriting", [&]() {
      foreach(pass::rw(rwName, rwK, rwZ))->transform(designBuilder);
    });
    passRw->add_option("--name", rwName);
    passRw->add_option("-k", rwK);
    passRw->add_flag("-z", rwZ);

    ADD_CMD(app, pass::rwz, "rwz", "Rewriting w/ zero-cost replacements");

    // Refactoring.
    ADD_CMD(app, pass::rf,  "rf",  "Refactoring");
    ADD_CMD(app, pass::rfz, "rfz", "Refactoring w/ zero-cost replacements");
    ADD_CMD(app, pass::rfa, "rfa", "Area-aware refactoring");
    ADD_CMD(app, pass::rfd, "rfd", "Depth-aware refactoring");
    ADD_CMD(app, pass::rfp, "rfp", "Power-aware refactoring");

    // Resubstitution.
    auto *passRs = ADD_CUSTOM_CMD(app,
        "rs", "Resubstitution", [&]() {
      foreach(pass::rs(rsName, rsK, rsN))->transform(designBuilder);
    });
    passRs->add_option("--name", rsName);
    passRs->add_option("-k", rsK);
    passRs->add_option("-n", rsN);

    auto *passRsz = ADD_CUSTOM_CMD(app,
        "rsz", "Resubstitution w/ zero-cost replacements", [&]() {
      foreach(pass::rsz(rszName, rszK, rszN))->transform(designBuilder);
    });
    passRsz->add_option("--name", rszName);
    passRsz->add_option("-k", rszK);
    passRsz->add_option("-n", rszN);

    // Predefined scripts.
    ADD_CMD(app, pass::resyn,     "resyn",     "Predefined script resyn");
    ADD_CMD(app, pass::resyn2,    "resyn2",    "Predefined script resyn2");
    ADD_CMD(app, pass::resyn2a,   "resyn2a",   "Predefined script resyn2a");
    ADD_CMD(app, pass::resyn3,    "resyn3",    "Predefined script resyn3");
    ADD_CMD(app, pass::compress,  "compress",  "Predefined script compress");
    ADD_CMD(app, pass::compress2, "compress2", "Predefined script compress2");

    app.require_subcommand();
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    namespace pass = eda::gate::optimizer;

    if (!designBuilder) {
      return makeError(interp, "design has not been loaded");
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    designBuilder->save("pass");
    previousStep = "pass";

    return TCL_OK;
  }

  // Rewriter.
  std::string rwName = "rw";
  uint16_t rwK = 4;
  bool rwZ = false;

  // Resubstitor.
  std::string rsName = "rs";
  uint16_t rsK = 8;
  uint16_t rsN = 16;

  // Resubstitutor w/ zero-cost replacements.
  std::string rszName = "rsz";
  uint16_t rszK = 8;
  uint16_t rszN = 16;
};

static LogOptCommand logOptCmd;

static int CmdLogOpt(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return logOptCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Read GraphML
//===----------------------------------------------------------------------===//

struct ReadGraphMlCommand final : public UtopiaCommand {
  ReadGraphMlCommand():
      UtopiaCommand("read_graphml", "Reads a GraphML file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (designBuilder) {
      return makeError(interp, "design has been already loaded");
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    std::string fileName = "";
    if (!app.remaining().empty()) {
      fileName = app.remaining().at(0);
    } else {
      return makeError(interp, "no input files");
    }

    if (!std::filesystem::exists(fileName)){
      return makeError(interp,
          fmt::format("file '{}' does not exist", fileName));
    }

    if (!fileName.empty()) {
      GmlTranslator::ParserData data;
      GmlTranslator parser;
      const auto &subnet = parser.translate(fileName, data)->make(true);
      designBuilder = std::make_unique<DesignBuilder>(subnet);
      designBuilder->save("original");
      return TCL_OK;
    }

    return makeError(interp, "no input files");
  }
};

static ReadGraphMlCommand readGraphMlCmd;

static int CmdReadGraphMl(
    ClientData,Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return readGraphMlCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Read Liberty
//===----------------------------------------------------------------------===//

struct ReadLibertyCommand final : public UtopiaCommand {
  ReadLibertyCommand():
      UtopiaCommand("read_liberty", "Reads a Liberty file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    std::string path = "";
    if (!app.remaining().empty()) {
      path = app.remaining().at(0);
    } else {
      return makeError(interp, "no input files");
    }

    if (!std::filesystem::exists(path)){
      return makeError(interp, fmt::format("file '{}' does not exist", path));
    }

    LibraryParser::get().loadLibrary(path);
    return TCL_OK;
  }
};

static ReadLibertyCommand readLibertyCmd;

static int CmdReadLiberty(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return readLibertyCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Read Verilog
//===----------------------------------------------------------------------===//

struct ReadVerilogCommand final : public UtopiaCommand {
  ReadVerilogCommand():
      UtopiaCommand("read_verilog", "Reads a Verilog file") {
    app.add_option("--frontend", frontend);
    app.add_option("--top", topModule);
    app.add_flag("--debug", debugMode);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (designBuilder) {
      return makeError(interp, "design has been already loaded");
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    std::string path = "";
    if (!app.remaining().empty()) {
      path = app.remaining().at(0);
    } else {
      return makeError(interp, "no input files");
    }

    if (!std::filesystem::exists(path)){
      return makeError(interp, fmt::format("file '{}' does not exist", path));
    }

    if (frontend == "yosys") {
      YosysToModel2Config cfg;
      cfg.debugMode = debugMode;
      cfg.topModule = topModule;
      cfg.files = app.remaining();

      YosysConverterModel2 cvt(cfg);      
      const auto &netId = cvt.getNetID();
      if (netId == OBJ_NULL_ID) {
        return makeError(interp, "null ID received");
      }

      designBuilder = std::make_unique<DesignBuilder>(netId);
      designBuilder->save("original");

      return TCL_OK;
    }

    return makeError(interp, fmt::format("unknown frontend '{}'", frontend));
  }
 
  std::string frontend = "yosys";
  std::string topModule;
  bool debugMode = false;
};

static ReadVerilogCommand readVerilogCmd;

static int CmdReadVerilog(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return readVerilogCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Design Statistics
//===----------------------------------------------------------------------===//

struct StatCommand final : public UtopiaCommand {
  StatCommand():
      UtopiaCommand("stat", "Prints the design characteristics") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    namespace estimator = eda::gate::estimator;

    if (!designBuilder) {
      return makeError(interp, "design has not been loaded");
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    size_t size{0}, depth{0};
    float  area{0}, delay{0}, power{0}, activ{0};

    for (size_t i = 0; i < designBuilder->getSubnetNum(); ++i) {
      const auto &subnetID = designBuilder->getSubnetID(i);
      const auto &subnet = Subnet::get(subnetID);

      /// FIXME: Use SubnetBuilder instead of Subnet.
      SubnetBuilder builder(subnet);
      eda::gate::analyzer::ProbabilityEstimator estimator;

      size  += subnet.getEntries().size();
      activ += estimator.estimate(builder).getSwitchProbsSum();
      depth = std::max<size_t>(subnet.getPathLength().second, depth);

      if (previousStep == "techmap") {
        area  += estimator::getArea(subnetID);
        power += estimator::getLeakagePower(subnetID);
        delay = std::max<float>(estimator::getArrivalTime(subnetID), delay);
      }
    } // for subnet

    UTOPIA_OUT << "Cells: " << size  << std::endl;
    UTOPIA_OUT << "Depth: " << depth << std::endl;
    UTOPIA_OUT << "Activ: " << activ << std::endl;

    if (previousStep == "techmap") {
      UTOPIA_OUT << "Area:  " << area  << std::endl;
      UTOPIA_OUT << "Delay: " << delay << std::endl;
      UTOPIA_OUT << "Power: " << power << std::endl;
    }

    return TCL_OK;
  }

  bool logic = false;
};

static StatCommand statCmd;

static int CmdStat(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return statCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Technology Mapping
//===----------------------------------------------------------------------===//
struct TechMapCommand final : public UtopiaCommand {
  TechMapCommand(): UtopiaCommand("techmap", "Performs technology mapping") {
    const std::map<std::string, Indicator> indicatorMap {
      { "area",  Indicator::AREA  },
      { "delay", Indicator::DELAY },
      { "power", Indicator::POWER },
    };

    app.add_option("--type", indicator, "Optimization criterion")
        ->expected(1)
        ->transform(CLI::CheckedTransformer(indicatorMap, CLI::ignore_case));
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (!designBuilder) {
      return makeError(interp, "design has not been loaded");
    }

    if (!LibraryParser::get().isInit()) {
      return makeError(interp, "library has not been loaded");
    }

    if (previousStep == "techmap") {
      return makeError(interp, "design has been already tech-mapped");
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    const size_t nSubnet = designBuilder->getSubnetNum();
    for (size_t subnetID = 0; subnetID < nSubnet; ++subnetID) {
      const auto &subnetBuilder = designBuilder->getSubnetBuilder(subnetID);
      const auto techmapBuilder = techMap(Objective(indicator), subnetBuilder);

      designBuilder->setSubnetBuilder(subnetID, techmapBuilder);
    }

    designBuilder->save("techmap");
    previousStep = "techmap";

    return TCL_OK;
  }

  Indicator indicator = Indicator::AREA;
};

static TechMapCommand techMapCmd;

static int CmdTechMap(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return techMapCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Verilog To FIRRTL
//===----------------------------------------------------------------------===//

struct VerilogToFirCommand final : public UtopiaCommand {
  VerilogToFirCommand():
      UtopiaCommand("verilog_to_fir", "Translates Verilog to FIRRTL") {
    app.add_flag("--debug", debugMode);
    app.add_option("--top", topModule);
    app.add_option("-o, --out", outputFile);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    if (app.remaining().empty()) {
      return makeError(interp, "no input files");
    }

    for (const auto &file: app.remaining()) {
      if (!std::filesystem::exists(file)) {
        return makeError(interp, fmt::format("file '{}' does not exist", file));
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

  std::string outputFile = "";
  std::string topModule;
  bool debugMode = false;
};

static VerilogToFirCommand verilogToFirCmd;

static int CmdVerilogToFir(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return verilogToFirCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Version
//===----------------------------------------------------------------------===//

struct VersionCommand final : public UtopiaCommand {
  VersionCommand():
      UtopiaCommand("version", "Prints Utopia EDA version") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_OUT << "Utopia EDA "
               << "version "
               << VERSION_MAJOR
               << "."
               << VERSION_MINOR
               << std::endl
               << std::flush;

    return TCL_OK;
  } 
};

static VersionCommand versionCmd;

static int CmdVersion(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return versionCmd.run/* simple */(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Write Design
//===----------------------------------------------------------------------===//

static void printModel(
    const std::string &fileName,
    const std::string &designName,
    ModelPrinter::Format format,
    const Net &net) {
  auto &printer = ModelPrinter::getPrinter(format);

  if (!fileName.empty()) {
    std::ofstream outFile(fileName);
    printer.print(outFile, net, designName);
    outFile.close();
  } else {
    printer.print(UTOPIA_OUT, net);
  }
}

struct WriteDesignCommand final : public UtopiaCommand {
  WriteDesignCommand():
      UtopiaCommand("write_design", "Writes the design to a file") {
    const std::map<std::string, ModelPrinter::Format> formatMap {
      { "verilog", ModelPrinter::VERILOG },
      { "simple",  ModelPrinter::SIMPLE  },
      { "dot",     ModelPrinter::DOT     },
    };

    app.add_option("-f, --format", format, "Output format")
        ->expected(1)
        ->transform(CLI::CheckedTransformer(formatMap, CLI::ignore_case));
    app.add_option("--path", fileName);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (!designBuilder) {
      return makeError(interp, "design has not been loaded");
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    if (format == ModelPrinter::VERILOG ||
        format == ModelPrinter::SIMPLE ||
        format == ModelPrinter::DOT) {
      const auto &net = Net::get(designBuilder->make());
      printModel(fileName, designName, format, net);
      return TCL_OK;
    }

    return makeError(interp, fmt::format("unknown format '{}'", format));
  }

  std::string fileName = "design.v";
  std::string designName = "design";
  ModelPrinter::Format format = ModelPrinter::VERILOG;
};

static WriteDesignCommand writeDesignCmd;

static int CmdWriteDesign(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return writeDesignCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Write Subnet
//===----------------------------------------------------------------------===//

static void printSubnet(
    const DesignBuilderPtr &designBuilder,
    size_t subnetID) {
  const auto &subnetBuilder = designBuilder->getSubnetBuilder(subnetID);
  const auto &subnet = Subnet::get(subnetBuilder->make(true));
  UTOPIA_OUT << subnet << std::endl;
}

struct WriteSubnetCommand final : public UtopiaCommand {
  WriteSubnetCommand():
      UtopiaCommand("write_subnet", "Writes a subnet to a file") {
    entered = app.add_option("-i, --index", number, "Subnet number");
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (!designBuilder) {
      return makeError(interp, "design has not been loaded");
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      return makeError(interp, e.what());
    }

    const size_t numSubnets = designBuilder->getSubnetNum();
    if (entered->count() == 0) {
      for (size_t subnetID = 0; subnetID < numSubnets; ++subnetID) {
        printSubnet(designBuilder, subnetID);
      }
    } else if (number < numSubnets) {
      printSubnet(designBuilder, number);
    } else {
      return makeError(interp, fmt::format("subnet {} does not exist", number));
    }

    return TCL_OK;
  }

  size_t number = 0;
  CLI::Option *entered = nullptr;
};

static WriteSubnetCommand writeSubnetCmd;

static int CmdWriteSubnet(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return writeSubnetCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Utopia Shell
//===----------------------------------------------------------------------===//

int Utopia_TclInit(Tcl_Interp *interp) {
  if ((Tcl_Init)(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  commandRegistry.addCommand(&dbstatCmd);
  commandRegistry.addCommand(&deleteCmd);
  commandRegistry.addCommand(&exitCmd);
  commandRegistry.addCommand(&helpCmd);
  commandRegistry.addCommand(&lecCmd);
  commandRegistry.addCommand(&logOptCmd);
  commandRegistry.addCommand(&readGraphMlCmd);
  commandRegistry.addCommand(&readLibertyCmd);
  commandRegistry.addCommand(&readVerilogCmd);
  commandRegistry.addCommand(&statCmd);
  commandRegistry.addCommand(&techMapCmd);
  commandRegistry.addCommand(&verilogToFirCmd);
  commandRegistry.addCommand(&versionCmd);
  commandRegistry.addCommand(&writeDesignCmd);
  commandRegistry.addCommand(&writeSubnetCmd);

#if 0
  Tcl_DeleteCommand(interp, "exec");
  Tcl_DeleteCommand(interp, "unknown");
#endif

  Tcl_CreateCommand(interp, dbstatCmd.name,       CmdDbStat,       NULL, NULL);
  Tcl_CreateCommand(interp, deleteCmd.name,       CmdDelete,       NULL, NULL);
  Tcl_CreateCommand(interp, helpCmd.name,         CmdHelp,         NULL, NULL);
  Tcl_CreateCommand(interp, lecCmd.name,          CmdLec,          NULL, NULL);
  Tcl_CreateCommand(interp, logOptCmd.name,       CmdLogOpt,       NULL, NULL);
  Tcl_CreateCommand(interp, readGraphMlCmd.name,  CmdReadGraphMl,  NULL, NULL);
  Tcl_CreateCommand(interp, readLibertyCmd.name,  CmdReadLiberty,  NULL, NULL);
  Tcl_CreateCommand(interp, readVerilogCmd.name,  CmdReadVerilog,  NULL, NULL);
  Tcl_CreateCommand(interp, statCmd.name,         CmdStat,         NULL, NULL);
  Tcl_CreateCommand(interp, techMapCmd.name,      CmdTechMap,      NULL, NULL);
  Tcl_CreateCommand(interp, verilogToFirCmd.name, CmdVerilogToFir, NULL, NULL);
  Tcl_CreateCommand(interp, versionCmd.name,      CmdVersion,      NULL, NULL);
  Tcl_CreateCommand(interp, writeDesignCmd.name,  CmdWriteDesign,  NULL, NULL);
  Tcl_CreateCommand(interp, writeSubnetCmd.name,  CmdWriteSubnet,  NULL, NULL);

  return TCL_OK;
}
