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
#include <memory>
#include <optional>
#include <unordered_map>
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
    Node(size_t type, size_t invIns) : type(type), invIns(invIns) {}

    size_t type;
    size_t invIns;
    std::optional<Link> link;
    std::vector<Input> inputs;
  };

  struct ParserData {
    std::unordered_map<size_t, Node> nodes;
    std::vector<Node*> groups[3]; // 3 groups: inputs, outputs, inner nodes
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
  std::shared_ptr<Builder> translate(const std::string &filename) const;

  /**
   * @brief Overloaded parse method that allows external access
   * to parser data after parsing is complete.
   * @param filename Absolute path to the GraphML file.
   * @param parserData Reference to ParserData to fill during parsing.
   * @return A SubnetBuilder for the built Subnet.
   */
  std::shared_ptr<Builder> translate(const std::string &filename,
                                     ParserData &data) const;

private:

  XMLElement* next(XMLElement* element) const {
    return element->NextSiblingElement();
  }

  XMLElement* findChild(XMLElement* element) const {
    return element->FirstChildElement();
  }

  bool checkName(XMLElement* element, const char* value) const {
    return !std::strcmp(element->Value(), value);
  }

  size_t getNum(XMLElement* element) const {
    return std::stoi(element->GetText());
  }

  size_t getGroup(XMLElement* element) const {
    size_t type = getNum(element);
    return type > 1 ? 2 : type;
  }

  void parseGraph(XMLElement *graph, ParserData &data) const;

  void parseNode(XMLElement *node, ParserData &data) const;

  void parseEdge(XMLElement *edge, ParserData &data) const;

  std::shared_ptr<Builder> buildSubnet(ParserData &data) const;

  void buildGroup(std::vector<Node*> &group, Builder *builder) const;

  const std::unordered_map<size_t, std::pair<model::CellSymbol, bool>> typeMap {
    {0, {model::IN, false}},
    {1, {model::OUT, false}},
    {2, {model::AND, false}},
    {10, {model::AND, true}},
    {11, {model::BUF, false}},
    {12, {model::BUF, true}},
    {13, {model::OR, false}},
    {14, {model::OR, true}},
    {15, {model::XOR, false}},
    {16, {model::XOR, true}},
    {1000, {model::ZERO, false}},
    {1001, {model::ONE, false}}
  };

};

} // namespace eda::gate::translator
