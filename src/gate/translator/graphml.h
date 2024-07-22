//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

#include "tinyxml2/tinyxml2.h"

#include <cstring>
#include <deque>
#include <memory>
#include <vector>

namespace eda::gate::translator {

/**
 * \brief Builds Subnet from GraphML/OpenABC-D description.
 */
class GmlTranslator {
public:
 
  using Builder     = eda::gate::model::SubnetBuilder;
  using LinkList    = eda::gate::model::Subnet::LinkList;
  using Link        = eda::gate::model::Subnet::Link;
  using Subnet      = eda::gate::model::Subnet;
  using XMLDocument = tinyxml2::XMLDocument;
  using XMLElement  = tinyxml2::XMLElement;

  struct Node;

  struct Input {
    Node *node;
    bool inv;
  };

  struct Node {
    Node(uint32_t id,  uint32_t type,  uint32_t invIns) :
      id(id), type(type), invIns(invIns) {}

    uint32_t id;
    uint32_t type;
    uint32_t invIns;
    std::vector<Input> inputs;
  };

  struct ParserData {
    std::deque<Node> nodes;
    std::vector<Node*> groups[3]; // 3 groups: inputs, inner nodes, outputs
  };

  /**
   * @brief Builds Subnet from the file with GraphML/OpenABC-D description.
   * The method can only parse GraphML files with the following constraints:
   *   - Node IDs must be unique and consistent throughout the file.
   *     They must start at 0 and have a step equal to 1. Nodes must have
   *     attributes: d0(node_id), d1(node_type), d2(num_inverted_predecessors).
   *     Example:
   * 
   *       <node id="0">
   *         <data key="d0">ys__n0</data>
   *         <data key="d1">0</data>
   *         <data key="d2">0</data>
   *       </node>
   * 
   *   - Edges must reference valid existing node IDs and have one attribute:
   *     d3(edge_type). Example:
   *        
   *       <edge source="2339" target="2338">
   *          <data key="d3">1</data>
   *       </edge>
   *
   *   - The description of the edges should follow after the description
   *     of all nodes.
   * @param filename Absolute path to the GraphML file.
   * @return A SubnetBuilder for the built Subnet.
   */
  std::shared_ptr<Builder> translate(const std::string &filename);

  /**
   * @brief Overloaded parse method that allows external access
   * to parser data after parsing is complete.
   * @param filename Absolute path to the GraphML file.
   * @param parserData Reference to ParserData to fill during parsing.
   * @return A SubnetBuilder for the built Subnet.
   */
  std::shared_ptr<Builder> translate(const std::string &filename,
                                     ParserData &data);

private:

  XMLElement* next(XMLElement* element) {
    return element->NextSiblingElement();
  }

  XMLElement* findChild(XMLElement* element) {
    return element->FirstChildElement();
  }

  bool checkName(XMLElement* element, const char* value) {
    return !std::strcmp(element->Value(), value);
  }

  uint32_t getNum(XMLElement* element) {
    return std::stoi(element->GetText());
  }

  void parseGraph(XMLElement *graph, ParserData &data);

  void parseNode(XMLElement *node, ParserData &data);

  void parseEdge(XMLElement *edge, ParserData &data);

  std::shared_ptr<Builder> buildSubnet(ParserData &data);
};

} // namespace eda::gate::translator
