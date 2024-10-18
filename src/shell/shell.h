//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "context/utopia_context.h"
#include "diag/logger.h"
#include "diag/terminal_printer.h"
#include "gate/model/design.h"

#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <tcl.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#define UTOPIA_SHELL_OUT std::cout
#define UTOPIA_SHELL_ERR std::cerr

#undef UTOPIA_SHELL_ENABLE_VERILOG_TO_FIR

#define UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv)\
  try {\
    app.parse(argc, argv);\
  } catch (CLI::ParseError &e) {\
    return makeError(interp, e.what());\
  }

#define UTOPIA_SHELL_WARN(interp, msg)\
  return makeWarn(interp, msg)

#define UTOPIA_SHELL_WARN_IF(interp, cond, msg)\
  if (cond) return makeWarn(interp, msg)

#define UTOPIA_SHELL_ERROR(interp, msg)\
  return makeError(interp, msg);

#define UTOPIA_SHELL_ERROR_IF(interp, cond, msg)\
  if (cond) return makeError(interp, msg)

#define UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp)\
  UTOPIA_SHELL_ERROR_IF(interp, !getDesign(),\
      "design has not been loaded")

#define UTOPIA_SHELL_ERROR_IF_DESIGN(interp)\
  UTOPIA_SHELL_ERROR_IF(interp, getDesign(),\
      "design has been already loaded")

#define UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app)\
  UTOPIA_SHELL_ERROR_IF(interp, app.remaining().empty(),\
      "no file(s) specified")

#define UTOPIA_SHELL_ERROR_IF_FILE_NOT_EXIST(interp, fileName)\
  UTOPIA_SHELL_ERROR_IF(interp, !std::filesystem::exists(fileName),\
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
  UTOPIA_SHELL_OUT << std::endl;
}

template <typename Clock>
inline void printTime(const std::string &name,
                      const std::chrono::time_point<Clock> &start,
                      const std::chrono::time_point<Clock> &end,
                      const std::string &prefix = "",
                      const std::string &suffix = "") {
  std::chrono::duration<double> elapsed = end - start;
  UTOPIA_SHELL_OUT << prefix << name << ": "
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
    UTOPIA_SHELL_OUT << line << std::endl;
  }

  file.close();
  return TCL_OK;
}

inline bool createDirs(const std::string &dir) {
  if (std::filesystem::exists(dir)) {
    return true;
  }
  std::error_code error;
  return std::filesystem::create_directories(dir, error);
}

inline int createParentDirs(Tcl_Interp *interp, const std::string &fileName) {
  std::filesystem::path filePath = fileName;

  UTOPIA_SHELL_ERROR_IF(interp, !filePath.has_filename(),
      "path does not contain a file name");

  const std::string dir = filePath.remove_filename();
  if (!dir.empty()) {
    UTOPIA_SHELL_ERROR_IF(interp, !createDirs(dir),
        fmt::format("cannot create directory '{}'", dir));
  }

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
  UtopiaCommand(const char *name, const char *desc, bool useDefTCLProc = false):
      name(name), desc(desc), app(desc, name), useDefaultTclProc(useDefTCLProc) {
    // CLI::App adds the help option, but it is not required.
    auto *helpOption = app.get_help_ptr();
    if (helpOption) {
      app.remove_option(helpOption);
    }
  }
  virtual ~UtopiaCommand() = default;
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

  void printHelp(std::ostream &out) const {
    out << app.help() << std::flush;
  }

  static int addCmd(ClientData clientData, Tcl_Interp *interp,
                    int argc, const char *argv[]) {
    return (reinterpret_cast<UtopiaCommand*>(clientData))->
              runEx (interp, argc, argv);
  }

  void setContext(eda::context::UtopiaContext *uContext) {
    assert(uContext);
    context = uContext;
  }

  const char *name;
  const char *desc;

  diag::Logger &logger{UTOPIA_LOGGER};
  diag::TerminalPrinter printer;

  UtopiaShell *shell;
  eda::context::UtopiaContext *context = nullptr;
  
  CLI::App app;
  bool useDefaultTclProc = false;
};

/**
 * @brief Utopia EDA shell.
 */
class UtopiaShell {
public:
  UtopiaShell();
  virtual std::string getName() const { return "Utopia EDA"; }

  virtual void printTitle(Tcl_Interp *interp);

  void addCommand(std::unique_ptr<UtopiaCommand> command) {
    assert(command);
    command->shell = this;
    commands.emplace(std::string(command->name), std::move(command));
  }

  void setContext(eda::context::UtopiaContext *context) {
    assert(context);
    for (const auto &[name, command] : commands) {
      command->setContext(context);
    }
  }

  UtopiaCommand *getCommand(const std::string &name) const {
    auto i = commands.find(name);
    return i != commands.cend() ? i->second.get() : nullptr;
  }

  const std::map<std::string, std::unique_ptr<UtopiaCommand>>
        &getCommands() const {
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

  virtual ~UtopiaShell() {}

  int appInitProc(Tcl_Interp *interp) const {
    if ((Tcl_Init)(interp) == TCL_ERROR) {
      return TCL_ERROR;
    }

    for (const auto &[name, command] : commands) {
      if (!command->useDefaultTclProc) {
        // Otherwise use the default processor (e.g. exit).
        Tcl_CreateCommand(interp, name.c_str(), command->addCmd,
                          reinterpret_cast<ClientData>(command.get()), nullptr);
      }
    } // [name, command]

    return TCL_OK;
  }

protected:
  std::map<std::string, std::unique_ptr<UtopiaCommand>> commands;
};

/// @brief Returns the design being synthesized.
std::shared_ptr<eda::gate::model::DesignBuilder> getDesign();

/// @brief Initializes the design from the cell type.
bool setDesign(const eda::gate::model::CellTypeID typeID, diag::Logger &logger);
/// @brief Initializes the design from the net.
bool setDesign(const eda::gate::model::NetID netID, diag::Logger &logger);
/// @brief Initializes the design from the subnet.
bool setDesign(const eda::gate::model::SubnetID subnetID, diag::Logger &logger);

/// @brief Deletes the design from memory.
void deleteDesign();

} // namespace eda::shell
