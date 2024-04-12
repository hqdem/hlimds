//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "CLI/CLI.hpp"
#include "gate/debugger/base_checker.h"
#include "gate/premapper/premapper.h"
#include "gate/techmapper/techmapper.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using Json = nlohmann::json;

NLOHMANN_JSON_SERIALIZE_ENUM( eda::gate::debugger::options::LecType, {
  {eda::gate::debugger::options::BDD, "bdd"},
  {eda::gate::debugger::options::RND, "rnd"},
  {eda::gate::debugger::options::SAT, "sat"},
})

NLOHMANN_JSON_SERIALIZE_ENUM( eda::gate::premapper::PreBasis, {
  {eda::gate::premapper::AIG, "aig"},
  {eda::gate::premapper::MIG, "mig"},
  {eda::gate::premapper::XAG, "xag"},
  {eda::gate::premapper::XMG, "xmg"},
})

class AppOptions {
public:
  AppOptions() = delete;

  AppOptions(const std::string &title,
             const std::string &version):
      isRoot(true), options(new CLI::App(title)) {}

  AppOptions(AppOptions &parent,
             const std::string &cmd,
             const std::string &desc):
      isRoot(false), options(parent.options->add_subcommand(cmd, desc)) {}

  virtual ~AppOptions() {
    if (isRoot) { delete options; }
  }

  void parse(int argc, char **argv) {
    options->parse(argc, argv);
  }

  virtual void fromJson(Json json) {
    // TODO: Default implementation.
  }

  virtual Json toJson() const {
    return toJson(options);
  }

  virtual void read(std::istream &in) {
    auto json = Json::parse(in);
    fromJson(json);
  }

  virtual void save(std::ostream &out) const {
    auto json = toJson();
    out << json;
  }

  void read(const std::string &config) {
    std::ifstream in(config);
    if (in.good()) {
      read(in);
    }
  }

  void save(const std::string &config) const {
    std::ofstream out(config);
    if (out.good()) {
      save(out);
    }
  }

protected:
  static std::string cli(const std::string &option) {
    return "--" + option;
  }

  static void get(Json json, const std::string &key, std::string &value) {
    if (json.contains(key)) {
      value = json[key].get<std::string>();
    }
  }

  template<class T>
  static void get(Json json, const std::string &key, T &value) {
    if (json.contains(key)) {
      value = json[key].get<T>();
    }
  }

  Json toJson(const CLI::App *app) const {
    Json json;

    for (const auto *opt : app->get_options({})) {
      if (!opt->get_lnames().empty() && opt->get_configurable()) {
        const auto name = opt->get_lnames()[0];

        if (opt->get_type_size() != 0) {
          if (opt->count() == 1) {
            json[name] = opt->results().at(0);
          } else if (opt->count() > 1) {
            json[name] = opt->results();
          } else if (!opt->get_default_str().empty()) {
            json[name] = opt->get_default_str();
          }
        } else if (opt->count() == 1) {
          json[name] = true;
        } else if (opt->count() > 1) {
          json[name] = opt->count();
        } else if (opt->count() == 0) {
          json[name] = false;
        }
      }
    }

    for(const auto *cmd : app->get_subcommands({})) {
      json[cmd->get_name()] = Json(toJson(cmd));
    }

    return json;
  }

  const bool isRoot;
  CLI::App *options;
};


struct RtlOptions final : public AppOptions {

  static constexpr const char *ID = "rtl";

  using LecType = eda::gate::debugger::options::LecType;
  using PreBasis = eda::gate::premapper::PreBasis;

  static constexpr const char *LEC_TYPE     = "lec";
  static constexpr const char *PREMAP_BASIS = "premap-basis";
  static constexpr const char *PREMAP_LIB   = "premap-lib";
  static constexpr const char *GRAPHML      = "graphml";

  const std::map<std::string, LecType> lecTypeMap {
    {"bdd", LecType::BDD},
    {"rnd", LecType::RND},
    {"sat", LecType::SAT},
  };

  const std::map<std::string, PreBasis> preBasisMap {
    {"aig", PreBasis::AIG},
    {"mig", PreBasis::MIG},
    {"xag", PreBasis::XAG},
    {"xmg", PreBasis::XMG}
  };

  RtlOptions(AppOptions &parent):
      AppOptions(parent, ID, "Logical synthesis") {

    // Named options.
    options->add_option(cli(LEC_TYPE), lecType, "Type of LEC")
           ->expected(1)
           ->transform(CLI::CheckedTransformer(lecTypeMap, CLI::ignore_case));
    options->add_option(cli(PREMAP_BASIS), preBasis, "Premapper basis")
           ->expected(1)
           ->transform(CLI::CheckedTransformer(preBasisMap, CLI::ignore_case));
    options->add_option(cli(PREMAP_LIB), preLib, "Premapper library")
           ->expected(1);
    options->add_option(cli(GRAPHML), graphMl,
                "Path to GraphML file for the model to be stored")
           ->expected(1);
    // Input file(s).
    options->allow_extras();
  }

  std::vector<std::string> files() const {
    return options->remaining();
  }

  void fromJson(Json json) override {
    get(json, LEC_TYPE, lecType);
    get(json, PREMAP_BASIS, preBasis);
    get(json, PREMAP_LIB, preLib);
    get(json, GRAPHML, graphMl);
  }

  LecType lecType = LecType::SAT;
  PreBasis preBasis = PreBasis::AIG;
  std::string preLib;
  std::string graphMl;
};

struct FirRtlOptions final : public AppOptions {

  /// The command to run the Verilog-to-FIRRTL translator.
  static constexpr const char *ID = "to_firrtl";
  /// The option to manually specify the top-level module. The top-level module is detected automatically if not specified.
  static constexpr const char *FIRRTL = "--top";
  /// The option to specify name of result translation.
  static constexpr const char *OUTPUT_NAMEFILE = "-o,--output";
  /// When debug mode is enabled, additional debug information may be generated in standard error output file.
  static constexpr const char *DEBUG_MODE = "-v,--verbose";

  FirRtlOptions(AppOptions &parent):
      AppOptions(parent, ID, "Translator from Verilog to FIRRTL") {

    options->add_option(FIRRTL, top,
                "Name of top module in Verilog")
            ->expected(1);
    options->add_option(OUTPUT_NAMEFILE, outputNamefile,
                "Name of output file")
            ->expected(1);
    options->add_flag(DEBUG_MODE, debugMode,
                "Enable debug mode");
    // Input Verilog file(s).
    options->allow_extras();
  }

  std::vector<std::string> files() const {
    return options->remaining();
  }

  void fromJson(Json json) override {
    get(json, FIRRTL, top);
  }

  std::string top;
  std::string outputNamefile;
  bool debugMode = false;
};

struct Model2Options final : public AppOptions {

  static constexpr const char *ID = "to_model2";
  static constexpr const char *NET = "--net";
  static constexpr const char *TOP = "--top";
  static constexpr const char *FIRRTL = "--fir";
  static constexpr const char *DEBUG_MODE = "--verbose";

  Model2Options(AppOptions &parent):
      AppOptions(parent, ID, "Translator from FIRRTL/Verilog to model2") {
    options->add_option(NET, outNetFileName, "Output Verilog file name")
           ->expected(1);
    options->add_option(TOP, topModuleName,
                        "Name of top module in Verilog")
           ->expected(1);
    options->add_option(FIRRTL, firrtlFileName,
                        "Name of FIRRTL file name")
           ->expected(1);
    options->add_flag(DEBUG_MODE, debugMode,
                "Enable debug mode");

    // Input Verilog file(s).
    options->allow_extras();
  }

  std::vector<std::string> files() const {
    return options->remaining();
  }

  void fromJson(Json json) override {
    get(json, NET, outNetFileName);
    get(json, TOP, topModuleName);
    get(json, FIRRTL, firrtlFileName);
  }

  std::string outNetFileName = "";
  std::string firrtlFileName = "temp.fir";
  std::string topModuleName = "";
  bool debugMode = false;
};

struct TechMapOptions final : public AppOptions {

  static constexpr const char *ID = "techmap";
  static constexpr const char *MAPPER_TYPE = "type";
  static constexpr const char *VERILOG_OUTPUT = "out";

  using MapperType = eda::gate::tech_optimizer::Techmapper::MapperType;
  const std::map<std::string, MapperType> mapperTypeMap {
    {"af", MapperType::AREA_FLOW},
    {"power", MapperType::POWER},
    {"delay", MapperType::DELAY},
    {"simple_area", MapperType::SIMPLE_AREA_FUNC},
    {"simple_delay", MapperType::SIMPLE_DELAY_FUNC},
  };

  TechMapOptions(AppOptions &parent):
      AppOptions(parent, ID, "Technological mapping") {

    // Named options.
    options->add_option(cli(MAPPER_TYPE), mapperType, "Type of mapper")
           ->expected(1)
           ->transform(CLI::CheckedTransformer(mapperTypeMap, CLI::ignore_case));

    options->add_option(cli(VERILOG_OUTPUT), outputPath,
                "Path to verilog file where the mapped design to be stored")
           ->expected(1);
    // Input file(s).
    options->allow_extras();
  }

  std::vector<std::string> files() const {
    return options->remaining();
  }

  void fromJson(Json json) override {
    get(json, MAPPER_TYPE, mapperType);
  }
  MapperType mapperType = MapperType::SIMPLE_AREA_FUNC;
  std::string outputPath = "out.v";
};

struct Options final : public AppOptions {
  Options(const std::string &title,
          const std::string &version):
      AppOptions(title, version), rtl(*this), firrtl(*this), model2(*this),
      techMapOptions(*this) {
    // Top-level options.
    options->set_help_all_flag("-H,--help-all",
                               "Print the extended help message and exit");
    options->set_version_flag("-v,--version", version,
                              "Print the tool version");
  }

  void initialize(const std::string &config, int argc, char **argv) {
    // Read the JSON configuration.
    read(config);
    // Command line is of higher priority.
    parse(argc, argv);
  }

  int exit(const CLI::Error &e) const {
    return options->exit(e);
  }

  void fromJson(Json json) override {
    rtl.fromJson(json[RtlOptions::ID]);
    firrtl.fromJson(json[FirRtlOptions::ID]);
    model2.fromJson(json[Model2Options::ID]);
    techMapOptions.fromJson(json[TechMapOptions::ID]); 
  }

  RtlOptions rtl;
  FirRtlOptions firrtl;
  Model2Options model2;
  TechMapOptions techMapOptions;
};
