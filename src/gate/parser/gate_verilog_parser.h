//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "util/logging.h"

#include <lorina/verilog.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace eda::gate::parser::verilog {

  /**
  * \brief Verilog parser based on Lorina.
  * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
  */
  class GateVerilogParser : public ::lorina::verilog_reader {
  private:
    struct ParserData {
      using GateId = eda::gate::model::GNet::GateId;
      using GateSymbol = eda::gate::model::GateSymbol;

      struct GateData {
        std::vector<GateId> inputs;
        GateId id = 0;
        GateSymbol kind = GateSymbol::ZERO;
      };

      struct LinkData {
        std::string target;
        std::vector<std::string> sources;
      };

      // Gate nickname / <its inputs, real net id, function>.
      // Nickname is how the gate is called in Verilog.
      std::unordered_map<std::string, GateData> gates;

      // Real net id / index in parser gates array.
      std::unordered_map<GateId, GateId> gIds;

      // Wire name / <source module name, target module names>.
      std::unordered_map<std::string, LinkData> links;

      // Output wires names.
      std::vector<std::string> outputs;

      // Name of the top-level net to be parsed.
      std::string netName;

      bool startParse = false;

      eda::gate::model::GNet *gnet = new eda::gate::model::GNet();
    };

    ParserData *data = new ParserData();

  public:
    explicit GateVerilogParser(std::string name);

    ~GateVerilogParser();

    eda::gate::model::GNet *getGnet();

    /*! \brief Callback method for parsed module.
     *
     * \param moduleName Name of the module
     * \param inputs Container for input and output names
     */
    void on_module_header(const std::string &moduleName,
                          const std::vector<std::string> &inputs) const override;

    /*! \brief Callback method for parsed inputs.
     *
     * \param inputs Input names
     * \param size Size modifier
     */
    void on_inputs(const std::vector<std::string> &inputs,
                   std::string const &size) const override;

    /*! \brief Callback method for parsed outputs.
     *
     * \param outputs Output names
     * \param size Size modifier
     */
    virtual void on_outputs(const std::vector<std::string> &outputs,
                            std::string const &size = "") const override;

    /*! \brief Callback method for parsed wires.
     *
     * \param wires Wire names
     * \param size Size modifier
     */
    void on_wires(const std::vector<std::string> &wires,
                  std::string const &size) const override;

    /*! \brief Callback method for parsed module instantiation of form `NAME
     * #(P1,P2) NAME(.SIGNAL(SIGNAL), ..., .SIGNAL(SIGNAL));`
     *
     * \param moduleName Name of the module
     * \param params List of parameters
     * \param instName Name of the instantiation
     * \param args List (a_1,b_1), ..., (a_n,b_n) of name pairs, where
     *             a_i is a name of a signals in moduleName and b_i is a name of
     * a signal in instName.
     */
    void on_module_instantiation(
            std::string const &moduleName,
            std::vector<std::string> const &params,
            std::string const &instName,
            std::vector<std::pair<std::string, std::string>> const &args) const override;

    /*! \brief Callback method for parsed immediate assignment of form `LHS = RHS ;`.
     *
     * \param lhs Left-hand side of assignment
     * \param rhs Right-hand side of assignment
     */
    virtual void on_assign(const std::string &lhs,
                           const std::pair<std::string, bool> &rhs) const override;

    /*! \brief Callback method for parsed endmodule.
     *
     */
    void on_endmodule() const override;

  private:
    void insertLink(const std::string &name,
                    const std::string &instName,
                    bool out) const;

    void reportNameError() const;

    ParserData::GateSymbol symbol(const std::string &s) const;
  };

  /**
  *  \brief Constructs a net from a given Verilog file.
  *  @param path full path to the Verilog file.
  *  @param netName Name of a net (Verilog module) that needs to be parsed.
  *  @return The constructed net.
  */
  eda::gate::model::GNet *getNet(const std::string &path,
                                 const std::string &netName);

} // namespace eda::gate::parser::verilog
