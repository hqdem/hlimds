//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/design_transformer.h"
#include "util/singleton.h"

#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <tcl.h>

#include <cassert>
#include <chrono>
#include <cmath> 
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define UTOPIA_OUT std::cout

#undef UTOPIA_ENABLE_VERILOG_TO_FIR

#define UTOPIA_PARSE_ARGS(interp, app, argc, argv)\
  try {\
    app.parse(argc, argv);\
  } catch (CLI::ParseError &e) {\
    return makeError(interp, e.what());\
  }

#define UTOPIA_ERROR_IF(interp, cond, message)\
  if (cond) return makeError(interp, message)

#define UTOPIA_ERROR_IF_NO_DESIGN(interp)\
  UTOPIA_ERROR_IF(interp, !designBuilder,\
      "design has not been loaded")

#define UTOPIA_ERROR_IF_DESIGN(interp)\
  UTOPIA_ERROR_IF(interp, designBuilder,\
      "design has been already loaded")

#define UTOPIA_ERROR_IF_NO_FILE(interp, fileName)\
  UTOPIA_ERROR_IF(interp, !std::filesystem::exists(fileName),\
      fmt::format("file '{}' does not exist", fileName))

//===----------------------------------------------------------------------===//
// Utility Functions
//===----------------------------------------------------------------------===//

template <typename Clock>
inline void printTime(const std::string &name,
                      const std::chrono::time_point<Clock> &start,
                      const std::chrono::time_point<Clock> &end,
                      const std::string &prefix = "",
                      const std::string &suffix = "") {
  std::chrono::duration<double> elapsed = end - start;
  UTOPIA_OUT << prefix << name << ": "
             << std::fixed << elapsed.count() << "s"
             << suffix << std::endl << std::flush;
}

inline int makeResult(Tcl_Interp *interp, const std::string &result) {
  Tcl_SetObjResult(interp, Tcl_NewStringObj(result.c_str(), -1));
  return TCL_OK;
}

inline int makeError(Tcl_Interp *interp, const std::string &error) {
  makeResult(interp, fmt::format("error: {}", error));
  return TCL_ERROR;
} 

//===----------------------------------------------------------------------===//
// Base Classes
//===----------------------------------------------------------------------===//

class UtopiaShell;

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

    const auto start = clock::now();
    const auto status = run(interp, argc, argv);
    const auto end = clock::now();

    printTime<clock>(fmt::format("{}({})", name, status), start, end, "> ");
    return status;
  }

  void printHelp(std::ostream &out) const {
    out << app.help() << std::flush;
  }

  const char *name;
  const char *desc;

  UtopiaShell *shell;

  CLI::App app;
};

/**
 * @brief Utopia EDA shell.
 */
class UtopiaShell : public eda::util::Singleton<UtopiaShell> {
  friend class eda::util::Singleton<UtopiaShell>;

public:
  virtual ~UtopiaShell() {}

  void addCommand(UtopiaCommand *command) {
    assert(command);
    commands.emplace(std::string(command->name), command);
    command->shell = this;
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
  UtopiaShell();

  std::map<std::string, UtopiaCommand*> commands;
};

extern eda::gate::optimizer::DesignBuilderPtr designBuilder;

extern int Utopia_TclInit(Tcl_Interp *interp, UtopiaShell &shell);

extern int Utopia_TclInit(Tcl_Interp *interp);
