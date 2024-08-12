//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/logger.h"
#include "diag/terminal_printer.h"
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
#define UTOPIA_ERR std::cerr

#undef UTOPIA_ENABLE_VERILOG_TO_FIR

#define UTOPIA_PARSE_ARGS(interp, app, argc, argv)\
  try {\
    app.parse(argc, argv);\
  } catch (CLI::ParseError &e) {\
    return makeError(interp, e.what());\
  }

#define UTOPIA_WARN(interp, msg)\
  return makeWarn(interp, msg)

#define UTOPIA_WARN_IF(interp, cond, msg)\
  if (cond) return makeWarn(interp, msg)

#define UTOPIA_ERROR(interp, msg)\
  return makeError(interp, msg);

#define UTOPIA_ERROR_IF(interp, cond, msg)\
  if (cond) return makeError(interp, msg)

#define UTOPIA_ERROR_IF_NO_DESIGN(interp)\
  UTOPIA_ERROR_IF(interp, !designBuilder,\
      "design has not been loaded")

#define UTOPIA_ERROR_IF_DESIGN(interp)\
  UTOPIA_ERROR_IF(interp, designBuilder,\
      "design has been already loaded")

#define UTOPIA_ERROR_IF_NO_FILES(interp, app)\
  UTOPIA_ERROR_IF(interp, app.remaining().empty(),\
      "no file(s) specified")

#define UTOPIA_ERROR_IF_FILE_NOT_EXIST(interp, fileName)\
  UTOPIA_ERROR_IF(interp, !std::filesystem::exists(fileName),\
      fmt::format("file '{}' does not exist", fileName))

namespace eda::shell {

//===----------------------------------------------------------------------===//
// Utility Functions
//===----------------------------------------------------------------------===//

inline int makeResult(Tcl_Interp *interp, const std::string &msg) {
  Tcl_SetObjResult(interp, Tcl_NewStringObj(msg.c_str(), -1));
  return TCL_OK;
}

inline int makeWarn(Tcl_Interp *interp, const std::string &msg) {
  makeResult(interp, fmt::format("warning: {}", msg));
  return TCL_ERROR;
}

inline int makeError(Tcl_Interp *interp, const std::string &msg) {
  makeResult(interp, fmt::format("error: {}", msg));
  return TCL_ERROR;
} 

inline void printNewline() {
  UTOPIA_OUT << std::endl;
}

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

template <typename Clock>
inline void printTimeAndEffect(const std::string &name,
                               const std::chrono::time_point<Clock> &start,
                               const std::chrono::time_point<Clock> &end,
                               const size_t oldCellNum,
                               const size_t newCellNum,
                               const std::string &prefix = "",
                               const std::string &suffix = "") {
  const auto delta = static_cast<int>(newCellNum) -
                     static_cast<int>(oldCellNum);

  const auto *sign = delta > 0 ? "+" : "";

  const auto percent = oldCellNum != 0 ?
      std::abs(100.f * delta / oldCellNum) : 0.f;

  printTime<Clock>(name, start, end, prefix,
      fmt::format(" -> {}{} [{:.2f}%] {}", sign, delta, percent, suffix));
}

inline int printFile(Tcl_Interp *interp, const std::string &filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    return makeError(interp, fmt::format("unable to open file '{}'", filePath));
  }

  std::string line;
  while (getline(file, line)) {
    UTOPIA_OUT << line << std::endl;
  }

  file.close();
  return TCL_OK;
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

    logger.getDiagnostics().initialize();

    const auto start = clock::now();
    const auto status = run(interp, argc, argv);
    const auto end = clock::now();

    printer.process(logger.getDiagnostics());

    printTime<clock>(fmt::format("{} [returned {}]", name, status),
        start, end, "> ");
    return status;
  }

  /// Returns a command processor pointer to be used in Tcl_CreateCommand.
  virtual Tcl_CmdProc *getCmdProc() const = 0;

  void printHelp(std::ostream &out) const {
    out << app.help() << std::flush;
  }

  const char *name;
  const char *desc;

  diag::Logger logger;
  diag::TerminalPrinter printer;

  UtopiaShell *shell;

  CLI::App app;
};

/*
 * @brief Utopia EDA shell command base class.
 */
template <typename Command, typename BaseCommand = UtopiaCommand>
struct UtopiaCommandBase : public BaseCommand,
                           public eda::util::Singleton<Command> {
  UtopiaCommandBase(const char *name, const char *desc):
      BaseCommand(name, desc) {}

  Tcl_CmdProc *getCmdProc() const override {
    return [](ClientData,
              Tcl_Interp *interp,
              int argc,
              const char *argv[]) -> int {
      auto &cmd = Command::get();
      return cmd.runEx(interp, argc, argv);
    };
  }
};

/**
 * @brief Utopia EDA shell.
 */
class UtopiaShell : public eda::util::Singleton<UtopiaShell> {
  friend class eda::util::Singleton<UtopiaShell>;

public:
  virtual std::string getName() const { return "Utopia EDA"; }

  virtual void printTitle(Tcl_Interp *interp);

  void addCommand(UtopiaCommand *command) {
    assert(command);
    commands.emplace(std::string(command->name), command);
    command->shell = this;
  }

  UtopiaCommand *getCommand(const std::string &name) const {
    auto i = commands.find(name);
    return i != commands.end() ? i->second : nullptr;
  }

  const std::map<std::string, UtopiaCommand*> &getCommands() const {
    return commands;
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

  virtual Tcl_AppInitProc *getAppInitProc() const {
    return UtopiaShell::getAppInitProc<UtopiaShell>();
  }

  virtual ~UtopiaShell() {}

protected:
  template <typename Shell>
  static Tcl_AppInitProc *getAppInitProc() {
    return [](Tcl_Interp *interp) -> int {
      if ((Tcl_Init)(interp) == TCL_ERROR) {
        return TCL_ERROR;
      }

      auto &shell = Shell::get();
      for (const auto &[name, command] : shell.getCommands()) {
        auto *proc = command->getCmdProc();

        if (proc) {
          // Otherwise use the default processor (e.g. exit).
          Tcl_CreateCommand(interp, name.c_str(), proc, nullptr, nullptr);
        }
      } // [name, command]

      return TCL_OK;
    };
  }

  UtopiaShell();

  std::map<std::string, UtopiaCommand*> commands;
};

/// @brief Design being synthesized.
extern eda::gate::optimizer::DesignBuilderPtr designBuilder;

} // namespace eda::shell

int umain(eda::shell::UtopiaShell &shell, int argc, char **argv);

int umain(int argc, char **argv);
