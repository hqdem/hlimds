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
#include <fmt/format.h>
#include <tcl.h>

#include <cassert>
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

struct UtopiaCommand {
  UtopiaCommand(const char *name, const char *desc):
    name(name), desc(desc), app(desc, name) {}

  virtual int run(Tcl_Interp *interp, int argc, const char *argv[]) = 0;

  void printHelp(std::ostream &out) const {
    out << app.help() << std::flush;
  }

  const char *name;
  const char *desc;

  CLI::App app;
};

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
// Clear
//===----------------------------------------------------------------------===//

struct ClearCommand final : public UtopiaCommand {
  ClearCommand():
      UtopiaCommand("clear", "Erases the design from memory") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    designBuilder = nullptr;
    return TCL_OK;
  }
};

static ClearCommand clear;

static int CmdClear(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return clear.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Database Statistics
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
      Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
      return TCL_ERROR;
    }

    if (app.remaining().empty()) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj("no files specified", -1));
      return TCL_ERROR;
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
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
        fmt::format("unknown output type '{}'", outputType).c_str(), -1));
      return TCL_ERROR;
    }

    config.outName = outputNamefile;
    config.ttSize = ttSize;
    config.binLines = app.remaining();

    if (getDbStat(UTOPIA_OUT, config)) {
      return TCL_ERROR;
    }

    return TCL_OK;
  }

  std::string dbPath;
  int ttSize;
  std::string outputType = "BOTH";
  std::string outputNamefile;
};

static DbStatCommand dbstat;

static int CmdDbStat(
    ClientData,
    Tcl_Interp *interp, int argc,
    const char *argv[]) {
  return dbstat.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Help
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
      Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
      return TCL_ERROR;
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

    Tcl_SetObjResult(interp, Tcl_NewStringObj(
        fmt::format("unknown command '{}'", name).c_str(), -1));
    return TCL_ERROR;
  }
};

static HelpCommand help;

static int CmdHelp(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return help.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// LEC
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
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "design has not been loaded", -1));
      return TCL_ERROR;
    }

    if (previousStep == "none") {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "no checkpoint to compare with", -1));
      return TCL_ERROR;
    }

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
        Tcl_SetObjResult(interp, Tcl_NewStringObj("unknown checker", -1));
        return TCL_ERROR;
    }

    const char *result = containsFalse(eq) ? "false" : "true";
    Tcl_SetObjResult(interp, Tcl_NewStringObj(result, -1));

    return TCL_OK;
  }

  LecType method = LecType::SAT;
};

static LecCommand lec;

static int CmdLec(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return lec.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Pass
//===----------------------------------------------------------------------===//

#define PARAM_SUBCOMMAND(app, cmd, func) do {\
  processSubcommand(app, #cmd, [&]() {\
    measureAndRun(#cmd, func);\
  });\
} while (false)

#define SUBCOMMAND(cli, cmd) do {\
  processSubcommand((cli), #cmd, [&]() {\
    measureAndRun(#cmd, [&]() {\
      foreach(pass::cmd())->transform(designBuilder);\
    });\
  });\
} while (false)

template<typename T>
static void processSubcommand(
    CLI::App &app,
    const std::string &subcommandName,
    T handler) {
  if (app.got_subcommand(subcommandName)) {
    handler();
  }
}

template<typename Func>
static void measureAndRun(const std::string &name, Func func) {
  auto start = std::chrono::high_resolution_clock::now();
  func();
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  UTOPIA_OUT << name << " took " << elapsed.count() << " seconds\n";
}

struct PassCommand final : public UtopiaCommand {
  PassCommand() :
      UtopiaCommand("pass", "Applies an optimization pass to the design") {
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
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    namespace pass = eda::gate::optimizer;

    if (!designBuilder) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "design has not been loaded", -1));
      return TCL_ERROR;
    }

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

static PassCommand pass;

static int CmdPass(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return pass.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Read GraphML
//===----------------------------------------------------------------------===//

struct ReadGraphMlCommand final : public UtopiaCommand {
  ReadGraphMlCommand():
      UtopiaCommand("read_graphml", "Reads a GraphML file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (designBuilder) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "design has already been loaded", -1));
      return TCL_ERROR;
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
      return TCL_ERROR;
    }

    std::string fileName = "";
    if (!app.remaining().empty()) {
      fileName = app.remaining().at(0);
    } else {
      Tcl_SetObjResult(interp, Tcl_NewStringObj("no files specified", -1));
      return TCL_ERROR;
    }

    if (!std::filesystem::exists(fileName)){
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
        fmt::format("file '{}' doesn't exist", fileName).c_str(), -1));
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

    Tcl_SetObjResult(interp, Tcl_NewStringObj("no files specified", -1));
    return TCL_ERROR;
  }
};

static ReadGraphMlCommand readGraphMl;

static int CmdReadGraphMl(
    ClientData,Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return readGraphMl.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Read Liberty
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
      Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
      return TCL_ERROR;
    }

    std::string path = "";
    if (!app.remaining().empty()) {
      path = app.remaining().at(0);
    } else {
      Tcl_SetObjResult(interp, Tcl_NewStringObj("no files specified", -1));
      return TCL_ERROR;
    }

    if (!std::filesystem::exists(path)){
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          fmt::format("File '{}' doesn't exist", path).c_str(), -1));
      return TCL_ERROR;
    }

    LibertyManager::get().loadLibrary(path);
    return TCL_OK;
  }
};

static ReadLibertyCommand readLiberty;

static int CmdReadLiberty(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return readLiberty.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Read Verilog
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
    if (!designBuilder) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "design has already been loaded", -1));
      return TCL_ERROR;
    }

    try {
      app.parse(argc, argv);
    } catch (CLI::ParseError &e) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
      return TCL_ERROR;
    }

    std::string path = "";
    if (!app.remaining().empty()) {
      path = app.remaining().at(0);
    } else {
      Tcl_SetObjResult(interp, Tcl_NewStringObj("no file specified", -1));
      return TCL_ERROR;
    }

    if (!std::filesystem::exists(path)){
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          fmt::format("file '{}' doesn't exist", path).c_str(), -1));
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

    Tcl_SetObjResult(interp, Tcl_NewStringObj(
      fmt::format("Unsupported frontend '{}'", frontend).c_str(), -1));
    return TCL_ERROR;
  }
 
  std::string frontend = "yosys";
  std::string topModule;
  bool debugMode = false;
};

static ReadVerilogCommand readVerilog;

static int CmdReadVerilog(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return readVerilog.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Statistics
//===----------------------------------------------------------------------===//

struct StatCommand final : public UtopiaCommand {
  StatCommand():
      UtopiaCommand("stat", "Prints the design characteristics") {
    app.add_flag("-l, --logical", logic, "Logic level characteristics");
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    namespace estimator = eda::gate::estimator;

    if (!designBuilder) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "design has not been loaded", -1));
      return TCL_ERROR;
    }

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

        UTOPIA_OUT << "Area: " <<
            subnet.getEntries().size() << '\n';
        UTOPIA_OUT << "Delay: " <<
            subnet.getPathLength().second << '\n';
        /// FIXME: Use SubnetBuilder instead of Subnet
        SubnetBuilder builder(subnet);
        UTOPIA_OUT << "Power: " <<
            estimator.estimate(builder).getSwitchProbsSum() << '\n';
      } else {
        if (previousStep != "techmap") {
          Tcl_SetObjResult(interp, Tcl_NewStringObj(
              "design has not been mapped", -1));
          return TCL_ERROR;
        }

        UTOPIA_OUT << "Area: " << estimator::getArea(id) << '\n';
        UTOPIA_OUT << "Delay: " << estimator::getArrivalTime(id) << '\n';
        UTOPIA_OUT << "Power: " << estimator::getLeakagePower(id) << '\n';
      }
    }

    return TCL_OK;
  }

  bool logic = false;
};

static StatCommand stat;

static int CmdStat(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return stat.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Technology Mapping
//===----------------------------------------------------------------------===//

struct TechMapCommand final : public UtopiaCommand {
  TechMapCommand(): UtopiaCommand("techmap", "Performs technology mapping") {
    const std::map<std::string, Techmapper::Strategy> mapperTypeMap {
      { "af",    Techmapper::Strategy::AREA_FLOW },
      { "area",  Techmapper::Strategy::AREA      },
      { "delay", Techmapper::Strategy::DELAY     },
      { "power", Techmapper::Strategy::POWER     },
    };

    app.add_option("--type", mapperType, "Type of mapper")
        ->expected(1)
        ->transform(CLI::CheckedTransformer(mapperTypeMap, CLI::ignore_case));
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (!designBuilder) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "design has not been loaded", -1));
      return TCL_ERROR;
    }

    if (LibertyManager::get().getLibraryName().empty()) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "library has not been loaded", -1));
      return TCL_ERROR;
    }

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

  Techmapper::Strategy mapperType = Techmapper::Strategy::AREA;
};

static TechMapCommand techMap;

static int CmdTechMap(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return techMap.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Verilog To FIRRTL
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
      Tcl_SetObjResult(interp, Tcl_NewStringObj(e.what(), -1));
      return TCL_ERROR;
    }

    if (app.remaining().empty()) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj("no files specified", -1));
      return TCL_ERROR;
    }

    for (const auto &file: app.remaining()) {
      if (!std::filesystem::exists(file)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(
            fmt::format("file '{}' doesn't exist", file).c_str(), -1));
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

  std::string outputFile = "out.fir";
  std::string topModule;
  bool debugMode = false;
};

static VerilogToFirCommand verilogToFir;

static int CmdVerilogToFir(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return verilogToFir.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Version
//===----------------------------------------------------------------------===//

struct VersionCommand final : public UtopiaCommand {
  VersionCommand():
      UtopiaCommand("version", "Prints Utopia EDA version") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_OUT << "Utopia EDA "
               << VERSION_MAJOR
               << "."
               << VERSION_MINOR
               << " | "
               << "Copyright (C) "
               << YEAR_STARTED << "-" << YEAR_CURRENT
               << " ISP RAS"
               << std::endl;

    return TCL_OK;
  } 
};

static VersionCommand version;

static int CmdVersion(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return version.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Write Design
//===----------------------------------------------------------------------===//

static void printModel(
    const std::string &fileName,
    ModelPrinter::Format format,
    const Net &net) {
  auto &printer = ModelPrinter::getPrinter(format);

  if (!fileName.empty()) {
    std::ofstream outFile(fileName);
    printer.print(outFile, net, "printedNet");
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

    app.add_option("--format", format, "Output format")
        ->expected(1)
        ->transform(CLI::CheckedTransformer(formatMap, CLI::ignore_case));
    app.add_option("--path", fileName);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (!designBuilder) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "design has not been loaded", -1));
      return TCL_ERROR;
    }

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

    Tcl_SetObjResult(interp, Tcl_NewStringObj(
        fmt::format("unknown format '{}'", format).c_str(), -1));
    return TCL_ERROR;
  }

  std::string fileName = "design.v";
  ModelPrinter::Format format = ModelPrinter::VERILOG;
};

static WriteDesignCommand writeDesign;

static int CmdWriteDesign(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return writeDesign.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Write Subnet
//===----------------------------------------------------------------------===//

static void printSubnet(
    const DesignBuilderPtr &designBuilder,
    size_t subnetId) {

  const auto &subnetBuilder = designBuilder->getSubnetBuilder(subnetId);
  const auto &subnet = Subnet::get(subnetBuilder->make(true));
  UTOPIA_OUT << subnet << std::endl;
}

struct WriteSubnetCommand final : public UtopiaCommand {
  WriteSubnetCommand():
      UtopiaCommand("write_subnet", "Writes a subnet to a file") {
    entered = app.add_option("--index", number, "Subnet number");
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    if (!designBuilder) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          "design has not been loaded", -1));
      return TCL_ERROR;
    }

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
      Tcl_SetObjResult(interp, Tcl_NewStringObj(
          fmt::format("subnet {} doesn't exist", number).c_str(), -1));
      return TCL_ERROR;
    }

    return TCL_OK;
  }

  size_t number = 0;
  CLI::Option *entered = nullptr;
};

static WriteSubnetCommand writeSubnet;

static int CmdWriteSubnet(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return writeSubnet.run(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Utopia Shell
//===----------------------------------------------------------------------===//

static inline void printNewline() {
  UTOPIA_OUT << std::endl;
}

static int printFile(Tcl_Interp *interp, const std::string &fileName) {
  const char *utopiaHome = std::getenv("UTOPIA_HOME");
  if (!utopiaHome) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
        "UTOPIA_HOME has not been set", -1));
    return TCL_ERROR;
  }

  std::string filePath = std::string(utopiaHome) + "/" + fileName;
  std::ifstream file(filePath);

  if (!file.is_open()) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
      fmt::format("unable to open file '{}'", filePath).c_str(), -1));
    return TCL_ERROR;
  }

  std::string line;
  while (getline(file, line)) {
    UTOPIA_OUT << line << std::endl;
  }

  file.close();
  return TCL_OK;
}

static inline int printTitle(Tcl_Interp *interp) {
  return printFile(interp, "doc/help/Title.txt");
}

static inline int printCopyright(Tcl_Interp *interp) {
  return printFile(interp, "doc/help/Copyright.txt");
}

int Utopia_TclInit(Tcl_Interp *interp) {
  if ((Tcl_Init)(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  commandRegistry.addCommand(&clear);
  commandRegistry.addCommand(&dbstat);
  commandRegistry.addCommand(&help);
  commandRegistry.addCommand(&lec);
  commandRegistry.addCommand(&pass);
  commandRegistry.addCommand(&readGraphMl);
  commandRegistry.addCommand(&readLiberty);
  commandRegistry.addCommand(&readVerilog);
  commandRegistry.addCommand(&stat);
  commandRegistry.addCommand(&techMap);
  commandRegistry.addCommand(&verilogToFir);
  commandRegistry.addCommand(&version);
  commandRegistry.addCommand(&writeDesign);
  commandRegistry.addCommand(&writeSubnet);

  printNewline();
  printTitle(interp);
  printNewline();
  printCopyright(interp);
  printNewline();

  Tcl_DeleteCommand(interp, "exec");
  Tcl_DeleteCommand(interp, "unknown");

  Tcl_CreateCommand(interp, clear.name,        CmdClear,        NULL, NULL);
  Tcl_CreateCommand(interp, dbstat.name,       CmdDbStat,       NULL, NULL);
  Tcl_CreateCommand(interp, help.name,         CmdHelp,         NULL, NULL);
  Tcl_CreateCommand(interp, lec.name,          CmdLec,          NULL, NULL);
  Tcl_CreateCommand(interp, pass.name,         CmdPass,         NULL, NULL);
  Tcl_CreateCommand(interp, readGraphMl.name,  CmdReadGraphMl,  NULL, NULL);
  Tcl_CreateCommand(interp, readLiberty.name,  CmdReadLiberty,  NULL, NULL);
  Tcl_CreateCommand(interp, readVerilog.name,  CmdReadVerilog,  NULL, NULL);
  Tcl_CreateCommand(interp, stat.name,         CmdStat,         NULL, NULL);
  Tcl_CreateCommand(interp, techMap.name,      CmdTechMap,      NULL, NULL);
  Tcl_CreateCommand(interp, verilogToFir.name, CmdVerilogToFir, NULL, NULL);
  Tcl_CreateCommand(interp, version.name,      CmdVersion,      NULL, NULL);
  Tcl_CreateCommand(interp, writeDesign.name,  CmdWriteDesign,  NULL, NULL);
  Tcl_CreateCommand(interp, writeSubnet.name,  CmdWriteSubnet,  NULL, NULL);

  return TCL_OK;
}
