//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/model/gsymbol.h"
#include "gate/printer/dot.h"
#include "util/logging.h"

#include <tinyxml2/tinyxml2.h>

#include <iostream>
#include <string>
#include <unordered_map>


namespace eda::gate::parser::graphml {

  using tinyxml2::XMLElement;
  using tinyxml2::XMLDocument;

  using GateId = eda::gate::model::GNet::GateId;
  using GateSymbol = eda::gate::model::GateSymbol;

/**
 * \brief Builds GNet from GraphML/OpenABC-D description.
 */
  class GraphMLParser {

  public:
    struct ParserData {
      struct GateData {
        struct InputData {
          int input;
          bool inverted;
        };
        int invertedNumber;
        GateId id;
        GateSymbol kind;
        std::vector<InputData> inputs;
      };

      std::unordered_map<int, GateData> gates;
      eda::gate::model::GNet *gnet = new eda::gate::model::GNet();
    };

    using InputData = ParserData::GateData::InputData;

    /**
    * \brief Builds GNet from the file with GraphML/OpenABC-D description.
    * The method can only parse GraphML files with the following constraints:
    *       - Node IDs must be unique and consistent throughout the file.
    *       - Edges must reference valid existing node IDs.
    *       - Nodes must have attributes: d0(node_id), d1(node_type),
    *         d2(num_inverted_predecessors).
    *       - Edges must have attributes: d3(edge_type).
    * \param filename Absolute path to the GraphML file.
    * \return A pointer to a GNet object representing the net.
    */
    static model::GNet *parse(const std::string &filename);

    /**
    * \brief Overloaded static parse method that allows external access
    * to parser data after parsing is complete.
    * \param filename Absolute path to the GraphML file.
    * \param parserData Reference to ParserData to fill during parsing.
    * \return A pointer to a GNet object representing the net.
    */
    static model::GNet *parse(const std::string &filename, ParserData &data);

  private:
    static XMLElement *findGraph(XMLElement *root);

    static void iterateFromGraphNode(XMLElement *graphNode, ParserData &data);

    static void parseNode(XMLElement *nodeElement, ParserData &data);

    static void parseEdge(XMLElement *edgeElement, ParserData &data);

    static void linkNet(ParserData &data);
  };

} // namespace eda::gate::parser::graphml