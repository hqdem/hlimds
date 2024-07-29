//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "config.h"
#include "gate/analyzer/probabilistic_estimate.h"
#include "gate/debugger/base_checker.h"
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
#include "shell.h"
#include "util/env.h"

#include <easylogging++.h>

#include <iostream>

using namespace eda::gate::model;
using namespace eda::gate::debugger;
using namespace eda::gate::debugger::options;
using namespace eda::gate::library;
using namespace eda::gate::optimizer;
using namespace eda::gate::techmapper;
using namespace eda::gate::translator;

INITIALIZE_EASYLOGGINGPP

DesignBuilderPtr designBuilder = nullptr;

//===----------------------------------------------------------------------===//
// Command: Delete Design
//===----------------------------------------------------------------------===//

struct DeleteDesignCommand final : public UtopiaCommand {
  DeleteDesignCommand():
      UtopiaCommand("delete_design", "Erases the design from memory") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    designBuilder = nullptr;
    return TCL_OK;
  }
};

static DeleteDesignCommand deleteDesignCmd;

static int CmdDeleteDesign(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return deleteDesignCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Goto Point
//===----------------------------------------------------------------------===//

struct GotoPointCommand final : public UtopiaCommand {
  GotoPointCommand():
      UtopiaCommand("goto_point", "Rolls back to a checkpoint") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().empty()) {
      return makeError(interp, "no point specified");
    }

    const auto point = app.remaining().at(0);
    designBuilder->rollback(point);

    return TCL_OK;
  }
};

static GotoPointCommand gotoPointCmd;

static int CmdGotoPoint(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return gotoPointCmd.runEx(interp, argc, argv);
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
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().empty()) {
      shell->printHelp(UTOPIA_OUT);
      return TCL_OK;
    }

    auto name = app.remaining().at(0);
    auto *command = shell->getCommand(name);

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
    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().size() < 2) {
      return makeError(interp, "no points specified");
    }

    const auto point1 = app.remaining().at(0);
    const auto point2 = app.remaining().at(1);

    if (point1 == point2) {
      return makeError(interp, "equal points specified");
    }
    if (!designBuilder->hasPoint(point1)) {
      return makeError(interp, fmt::format("unknown point '{}'", point1));
    }
    if (!designBuilder->hasPoint(point2)) {
      return makeError(interp, fmt::format("unknown point '{}'", point2));
    }

    const auto &checker = BaseChecker::getChecker(method);
    const bool verdict = checker.areEquivalent(
        *designBuilder, point1, point2).equal();

    const char *result = verdict ? "equivalent" : "not equivalent";
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
// Command: List Points
//===----------------------------------------------------------------------===//

struct ListPointsCommand final : public UtopiaCommand {
  ListPointsCommand():
      UtopiaCommand("list_points", "Lists the design checkpoints") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_NO_DESIGN(interp);

    const auto points = designBuilder->getPoints();

    if (points.empty()) {
      UTOPIA_OUT << "  <empty>" << std::endl << std::flush;
      return TCL_OK;
    }

    for (const auto &point : points) {
      UTOPIA_OUT << "  - " << point << std::endl;
    }
    UTOPIA_OUT << std::flush;

    return TCL_OK;
  }
};

static ListPointsCommand listPointsCmd;

static int CmdListPoints(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return listPointsCmd.runEx(interp, argc, argv);
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

static inline size_t getCellNum(const DesignBuilder &design) {
  size_t nCell{0};
  for (size_t i = 0; i < designBuilder->getSubnetNum(); ++i) {
    const auto &subnet = Subnet::get(designBuilder->getSubnetID(i));
    nCell += subnet.getCellNum();
  }
  return nCell;
}

template<typename Func>
static void measureAndRun(const std::string &name, Func func) {
  using clock = std::chrono::high_resolution_clock;

  const auto oldCellNum = getCellNum(*designBuilder);
  const auto start = clock::now();
  func();
  const auto end = clock::now();
  const auto newCellNum = getCellNum(*designBuilder);

  const auto delta = static_cast<int>(newCellNum) -
                     static_cast<int>(oldCellNum);

  const auto *sign = delta > 0 ? "+" : "";

  const auto percent = std::abs(100.f * delta / oldCellNum);

  printTime<clock>(name, start, end,
      /* prefix */ "  - ",
      /* suffix */ fmt::format(" -> {}{} [{:.2f}%]", sign, delta, percent));
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
    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    // Passes are executed as callbacks when parsing the arguments.
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
      UtopiaCommand("read_graphml", "Reads a design from a GraphML file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    std::string fileName = "";
    if (!app.remaining().empty()) {
      fileName = app.remaining().at(0);
    } else {
      return makeError(interp, "no input files");
    }

    UTOPIA_ERROR_IF_NO_FILE(interp, fileName);

    GmlTranslator::ParserData data;
    GmlTranslator parser;
    const auto &subnet = parser.translate(fileName, data)->make(true);
    designBuilder = std::make_unique<DesignBuilder>(subnet);

    return TCL_OK;
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
      UtopiaCommand("read_liberty", "Reads a library from a Liberty file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    std::string fileName = "";
    if (!app.remaining().empty()) {
      fileName = app.remaining().at(0);
    } else {
      return makeError(interp, "no input files");
    }

    UTOPIA_ERROR_IF_NO_FILE(interp, fileName);

    LibraryParser::get().loadLibrary(fileName);
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
      UtopiaCommand("read_verilog", "Reads a design from a Verilog file") {
    app.add_option("--frontend", frontend);
    app.add_option("--top", topModule);
    app.add_flag("--debug", debugMode);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    std::string fileName = "";
    if (!app.remaining().empty()) {
      fileName = app.remaining().at(0);
    } else {
      return makeError(interp, "no input files");
    }

    UTOPIA_ERROR_IF_NO_FILE(interp, fileName);

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
// Command: Save Point
//===----------------------------------------------------------------------===//

struct SavePointCommand final : public UtopiaCommand {
  SavePointCommand():
      UtopiaCommand("save_point", "Saves the design checkpoint") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().empty()) {
      return makeError(interp, "no point specified");
    }

    const auto point = app.remaining().at(0);
    designBuilder->save(point);

    return TCL_OK;
  }
};

static SavePointCommand savePointCmd;

static int CmdSavePoint(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return savePointCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Set Design Name
//===----------------------------------------------------------------------===//

struct SetNameCommand final : public UtopiaCommand {
  SetNameCommand():
      UtopiaCommand("set_name", "Sets the design name") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().empty()) {
      return makeError(interp, "no name specified");
    }

    const auto name = app.remaining().at(0);
    designBuilder->setName(name);

    return TCL_OK;
  }
};

static SetNameCommand setNameCmd;

static int CmdSetName(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return setNameCmd.run/* simple */(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Statistics for Design
//===----------------------------------------------------------------------===//

template <typename T>
static inline void printNameValue(const std::string &name,
                                  const T &value,
                                  const std::string &suffix = "") {
  UTOPIA_OUT << std::setw(8) << std::left << name
             << std::fixed << value
             << suffix
             << std::endl;
}

struct StatDesignCommand final : public UtopiaCommand {
  StatDesignCommand():
      UtopiaCommand("stat_design", "Prints the design characteristics") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    namespace estimator = eda::gate::estimator;

    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    const bool isTechMapped = designBuilder->isTechMapped();

    size_t nIn{0}, nOut{0}, nCell{0}, depth{0};
    float area{0}, delay{0}, power{0}, activ{0};

    for (size_t i = 0; i < designBuilder->getSubnetNum(); ++i) {
      const auto &subnetID = designBuilder->getSubnetID(i);
      const auto &subnet = Subnet::get(subnetID);

      /// FIXME: Use SubnetBuilder instead of Subnet.
      SubnetBuilder builder(subnet);
      eda::gate::analyzer::ProbabilityEstimator estimator;

      nIn += subnet.getInNum();
      nOut += subnet.getOutNum();
      nCell += subnet.getCellNum();
      activ += estimator.estimate(builder).getSwitchProbsSum();
      depth = std::max<size_t>(subnet.getPathLength().second, depth);

      if (isTechMapped) {
        area += estimator::getArea(subnetID);
        power += estimator::getLeakagePower(subnetID);
        delay = std::max<float>(estimator::getArrivalTime(subnetID), delay);
      }
    } // for subnet

    printNameValue("Name", designBuilder->getName());
    printNameValue("PIs", nIn);
    printNameValue("POs", nOut);
    printNameValue("Subnets", designBuilder->getSubnetNum());
    printNameValue("Cells", nCell, " (incl. PI/PO)");
    printNameValue("Depth", depth);
    printNameValue("SwAct", activ);

    if (isTechMapped) {
      printNameValue("Area", area, " um^2");
      printNameValue("Delay", delay, " ns");
      printNameValue("Power", power, " uW");
    }

    UTOPIA_OUT << std::flush;
    return TCL_OK;
  }
};

static StatDesignCommand statDesignCmd;

static int CmdStatDesign(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return statDesignCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Command: Statistics for Logical Optimization Database
//===----------------------------------------------------------------------===//

struct StatLogDbCommand final : public UtopiaCommand {
  StatLogDbCommand():
      UtopiaCommand("stat_logdb", "Prints information about a logopt database") {
    app.add_option("--db", dbPath)->expected(1)->required(true);
    app.add_option("--otype", outputType)->expected(1);
    app.add_option("--out", outputNamefile)->expected(1);
    app.add_option("--ttsize", ttSize)->expected(1)->required(true);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

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

static StatLogDbCommand statLogDbCmd;

static int CmdStatLogDb(
    ClientData,
    Tcl_Interp *interp, int argc,
    const char *argv[]) {
  return statLogDbCmd.runEx(interp, argc, argv);
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
    UTOPIA_ERROR_IF_NO_DESIGN(interp);

    if (!LibraryParser::get().isInit()) {
      return makeError(interp, "library has not been loaded");
    }

    if (designBuilder->isTechMapped()) {
      return makeError(interp, "design has been already techmapped");
    }

    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    for (size_t i = 0; i < designBuilder->getSubnetNum(); ++i) {
      const auto &subnetBuilder = designBuilder->getSubnetBuilder(i);
      const auto techmapBuilder = techMap(Objective(indicator), subnetBuilder);

      if (!techmapBuilder) {
        return makeError(interp, "returned null");
      }

      designBuilder->setSubnetBuilder(i, techmapBuilder);
    }

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

#ifdef UTOPIA_ENABLE_VERILOG_TO_FIR
struct VerilogToFirCommand final : public UtopiaCommand {
  VerilogToFirCommand():
      UtopiaCommand("verilog_to_fir", "Translates Verilog to FIRRTL") {
    app.add_flag("--debug", debugMode);
    app.add_option("--top", topModule);
    app.add_option("-o, --out", outputFile);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().empty()) {
      return makeError(interp, "no input files");
    }

    for (const auto &fileName: app.remaining()) {
      UTOPIA_ERROR_IF_NO_FILE(interp, fileName);
    }

    FirrtlConfig cfg;
    cfg.debugMode = debugMode;
    cfg.outputFileName = outputFile;
    cfg.topModule = topModule;
    cfg.files = app.remaining();
    YosysConverterFirrtl converter(cfg);

    return TCL_OK;
  } 

  std::string outputFile;
  std::string topModule;
  bool debugMode{false};
};

static VerilogToFirCommand verilogToFirCmd;

static int CmdVerilogToFir(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return verilogToFirCmd.runEx(interp, argc, argv);
}
#endif // UTOPIA_ENABLE_VERILOG_TO_FIR

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
// Command: Write Dot/Simple/Verilog
//===----------------------------------------------------------------------===//

static inline void printDesign(std::ostream &out,
                               DesignBuilder &designBuilder,
                               ModelPrinter &printer) {
  const auto &net = Net::get(designBuilder.make());

  const auto designName = designBuilder.getName();
  printer.print(out, net, designName);
}

static inline void printSubnet(std::ostream &out,
                               DesignBuilder &designBuilder,
                               size_t i,
                               ModelPrinter &printer) {
  const auto subnetID = designBuilder.getSubnetID(i);
  const auto &subnet = Subnet::get(subnetID);

  const auto subnetName = fmt::format("{}_{}", designBuilder.getName(), i);
  printer.print(out, subnet, subnetName);
}

struct WriteDesignCommand : public UtopiaCommand {
  using Format = ModelPrinter::Format;

  WriteDesignCommand(const char *name, const char *desc, const Format format):
      UtopiaCommand(name, desc), printer(ModelPrinter::getPrinter(format)) {
    subnetOption = app.add_option("--subnet", subnetIndex, "Subnet index");
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    std::string fileName = "";
    if (!app.remaining().empty()) {
      fileName = app.remaining().at(0);
    } else {
      return makeError(interp, "no input files");
    }

    std::ofstream out(fileName);

    if (subnetOption->count() == 0) {
      printDesign(out, *designBuilder, printer);
      return TCL_OK;
    }
    if (subnetIndex < designBuilder->getSubnetNum()) {
      printSubnet(out, *designBuilder, subnetIndex, printer);
      return TCL_OK;
    }

    return makeError(interp,
        fmt::format("subnet {} does not exist", subnetIndex));
  }

  ModelPrinter &printer;
  CLI::Option *subnetOption;
  size_t subnetIndex;
};

struct WriteDebugCommand final : public WriteDesignCommand {
   WriteDebugCommand():
       WriteDesignCommand("write_debug",
                          "Writes the design to a debug file",
                          Format::SIMPLE) {}
};

static WriteDebugCommand writeDebugCmd;

static int CmdWriteDebug(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return writeDebugCmd.runEx(interp, argc, argv);
}

struct WriteDotCommand final : public WriteDesignCommand {
  WriteDotCommand():
      WriteDesignCommand("write_dot",
                         "Writes the design to a DOT file",
                         Format::DOT) {}
};

static WriteDotCommand writeDotCmd;

static int CmdWriteDot(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return writeDotCmd.runEx(interp, argc, argv);
}

struct WriteVerilogCommand final : public WriteDesignCommand {
   WriteVerilogCommand():
       WriteDesignCommand("write_verilog",
                          "Writes the design to a Verilog file",
                          Format::VERILOG) {}
};

static WriteVerilogCommand writeVerilogCmd;

static int CmdWriteVerilog(
    ClientData,
    Tcl_Interp *interp,
    int argc,
    const char *argv[]) {
  return writeVerilogCmd.runEx(interp, argc, argv);
}

//===----------------------------------------------------------------------===//
// Utopia Shell
//===----------------------------------------------------------------------===//

UtopiaShell::UtopiaShell() {
  addCommand(&deleteDesignCmd);
  addCommand(&gotoPointCmd);
  addCommand(&exitCmd);
  addCommand(&helpCmd);
  addCommand(&lecCmd);
  addCommand(&listPointsCmd);
  addCommand(&logOptCmd);
  addCommand(&readGraphMlCmd);
  addCommand(&readLibertyCmd);
  addCommand(&readVerilogCmd);
  addCommand(&savePointCmd);
  addCommand(&setNameCmd);
  addCommand(&statDesignCmd);
  addCommand(&statLogDbCmd);
  addCommand(&techMapCmd);
#ifdef UTOPIA_ENABLE_VERILOG_TO_FIR
  addCommand(&verilogToFirCmd);
#endif // UTOPIA_ENABLE_VERILOG_TO_FIR
  addCommand(&versionCmd);
  addCommand(&writeDebugCmd);
  addCommand(&writeDotCmd);
  addCommand(&writeVerilogCmd);
}

int Utopia_TclInit(Tcl_Interp *interp, UtopiaShell &shell) {
  if ((Tcl_Init)(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

#if 0
  Tcl_DeleteCommand(interp, "exec");
  Tcl_DeleteCommand(interp, "unknown");
#endif

  Tcl_CreateCommand(interp, deleteDesignCmd.name, CmdDeleteDesign,  NULL, NULL);
  Tcl_CreateCommand(interp, gotoPointCmd.name,    CmdGotoPoint,     NULL, NULL);
  Tcl_CreateCommand(interp, helpCmd.name,         CmdHelp,          NULL, NULL);
  Tcl_CreateCommand(interp, lecCmd.name,          CmdLec,           NULL, NULL);
  Tcl_CreateCommand(interp, listPointsCmd.name,   CmdListPoints,    NULL, NULL);
  Tcl_CreateCommand(interp, logOptCmd.name,       CmdLogOpt,        NULL, NULL);
  Tcl_CreateCommand(interp, readGraphMlCmd.name,  CmdReadGraphMl,   NULL, NULL);
  Tcl_CreateCommand(interp, readLibertyCmd.name,  CmdReadLiberty,   NULL, NULL);
  Tcl_CreateCommand(interp, readVerilogCmd.name,  CmdReadVerilog,   NULL, NULL);
  Tcl_CreateCommand(interp, savePointCmd.name,    CmdSavePoint,     NULL, NULL);
  Tcl_CreateCommand(interp, setNameCmd.name,      CmdSetName,       NULL, NULL);
  Tcl_CreateCommand(interp, statDesignCmd.name,   CmdStatDesign,    NULL, NULL);
  Tcl_CreateCommand(interp, statLogDbCmd.name,    CmdStatLogDb,     NULL, NULL);
  Tcl_CreateCommand(interp, techMapCmd.name,      CmdTechMap,       NULL, NULL);
#ifdef UTOPIA_ENABLE_VERILOG_TO_FIR
  Tcl_CreateCommand(interp, verilogToFirCmd.name, CmdVerilogToFir,  NULL, NULL);
#endif // UTOPIA_ENABLE_VERILOG_TO_FIR
  Tcl_CreateCommand(interp, versionCmd.name,       CmdVersion,      NULL, NULL);
  Tcl_CreateCommand(interp, writeDebugCmd.name,    CmdWriteDebug,   NULL, NULL);
  Tcl_CreateCommand(interp, writeDotCmd.name,      CmdWriteDot,     NULL, NULL);
  Tcl_CreateCommand(interp, writeVerilogCmd.name,  CmdWriteVerilog, NULL, NULL);

  return TCL_OK;
}

int Utopia_TclInit(Tcl_Interp *interp) {
  return Utopia_TclInit(interp, UtopiaShell::get());
}

static int printUtopiaFile(Tcl_Interp *interp, const std::string &fileName) {
  const char *utopiaHome = std::getenv("UTOPIA_HOME");
  if (!utopiaHome) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
        "UTOPIA_HOME has not been set", -1));
    return TCL_ERROR;
  }

  std::string filePath = std::string(utopiaHome) + "/" + fileName;
  return printFile(interp, filePath);
}

static inline int printTitle(Tcl_Interp *interp) {
  return printUtopiaFile(interp, "doc/help/Title.txt");
}

static inline int printCopyright(Tcl_Interp *interp) {
  return printUtopiaFile(interp, "doc/help/Copyright.txt");
}

static inline void printTitleCopyright(Tcl_Interp *interp) {
  printNewline();
  printTitle(interp);
  printNewline();
  printCopyright(interp);
  printNewline();
}

int Utopia_Main(Tcl_AppInitProc init, int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  CLI::App app{"Utopia EDA"};

  std::string path = "";
  std::string script = "";
  bool interactiveMode = false;
  bool exitAfterEval = false;
  auto *fileMode = app.add_option(
      "-s, --script",
      path,
      "Executes a TCL script from a file");
  auto *evalMode = app.add_option(
      "-e, --evaluate",
      script,
      "Executes a TCL script from the terminal");
  app.add_flag(
      "-i, --interactive",
      interactiveMode,
      "Enters to interactive mode");
  app.allow_extras();

  CLI11_PARSE(app, argc, argv);

  Tcl_FindExecutable(argv[0]);
  Tcl_Interp *interp = Tcl_CreateInterp();
  if (init(interp) == TCL_ERROR) {
    std::cerr << "Failed to init Tcl interpreter\n";
    return 1;
  }
  printTitleCopyright(interp);

  int rc = 0;
  if (fileMode->count()) {
    std::vector<std::string> scriptArgs = app.remaining();
    const char *fileName = path.c_str();

    Tcl_Obj *tclArgv0 = Tcl_NewStringObj(fileName, -1);
    Tcl_SetVar2Ex(interp, "argv0", nullptr, tclArgv0, TCL_GLOBAL_ONLY);

    Tcl_Obj *tclArgvList = Tcl_NewListObj(0, nullptr);
    for (const auto &arg : scriptArgs) {
      Tcl_ListObjAppendElement(
          interp,
          tclArgvList,
          Tcl_NewStringObj(arg.c_str(), -1));
    }

    Tcl_Obj *tclArgc =
        Tcl_NewLongObj(static_cast<long>(scriptArgs.size()));
    Tcl_SetVar2Ex(interp, "argc", nullptr, tclArgc, TCL_GLOBAL_ONLY);
    Tcl_SetVar2Ex(interp, "argv", nullptr, tclArgvList, TCL_GLOBAL_ONLY);

    if (Tcl_EvalFile(interp, fileName) == TCL_ERROR) {
      std::cerr << Tcl_GetStringResult(interp) << '\n';
      rc = 1;
    }
    exitAfterEval = true;
  } else if (evalMode->count()) {
    if (Tcl_Eval(interp, script.c_str()) == TCL_ERROR) {
      std::cerr << Tcl_GetStringResult(interp) << '\n';
      rc = 1;
    }
    exitAfterEval = true;
  }
  if (interactiveMode || !exitAfterEval) {
    Tcl_MainEx(argc, argv, init, interp);
  }

  Tcl_DeleteInterp(interp);
  Tcl_Finalize();
  return rc;
}

int Utopia_Main(int argc, char **argv) {
  return Utopia_Main(Utopia_TclInit, argc, argv);
}
