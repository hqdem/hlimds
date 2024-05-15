//===----------------------------------------------------------------------===//GraphMLToSubnetPa
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===

#include "graphml_to_subnet.h"
#include "util/logging.h"

#include <tinyxml2/tinyxml2.h>

#include <iostream>

namespace eda::gate::parser::graphml {

using tinyxml2::XMLElement;
using tinyxml2::XMLDocument;

using ParserData = GraphMlSubnetParser::ParserData;
using SubnetID = GraphMlSubnetParser::SubnetID;

XMLElement *findGraph(XMLElement *root) {
  XMLElement *child = root->FirstChildElement();
  while (child) {
    const char *tagName = child->Value();
    if (tagName && !std::strcmp(tagName, "graph")) {
      return child;
    }
    child = child->NextSiblingElement();
  }
  LOG_ERROR << "Graph Node is not found" << std::endl;
  return nullptr;
}

SubnetID GraphMlSubnetParser::parse(const std::string &filename) {
  ParserData data;
  return parse(filename, data);
}

SubnetID GraphMlSubnetParser::parse(const std::string &filename,
                                    ParserData &data) {
  XMLDocument doc;
  doc.LoadFile(filename.c_str());
  uassert(!doc.ErrorID(), "Error loading file" << std::endl);

  XMLElement *root = doc.RootElement();
  uassert(root, "No root element found in file" << std::endl);

  XMLElement *graph = findGraph(root);
  uassert(graph, "No graph element found in file" << std::endl);

  parseGraph(graph, data);

  return buildSubnet(data);
}

void GraphMlSubnetParser::parseGraph(XMLElement *graph, ParserData &data) {
  XMLElement *element = findChild(graph);

  for (; checkName(element, "node"); element = next(element)) {
    parseNode(element, data);
  }

  for (; element; element = next(element)) {
    parseEdge(element, data);
  }
}

void GraphMlSubnetParser::parseNode(XMLElement *node, ParserData &data) {
  uint32_t id = std::stoi(node->Attribute("id"));

  XMLElement *type = next(findChild(node));
  XMLElement *invIns = next(type);

  data.nodes.emplace_back(id, getNum(type), getNum(invIns));
  data.groups[getNum(type)].push_back(&data.nodes.back());
}

void GraphMlSubnetParser::parseEdge(XMLElement *edge, ParserData &data) {
  Node &sourceNode = data.nodes.at(std::stoi(edge->Attribute("target")));
  Node &targetNode = data.nodes.at(std::stoi(edge->Attribute("source")));

  targetNode.inputs.push_back({sourceNode.id, 
                                static_cast<bool>(getNum(findChild(edge)))});
}

SubnetID GraphMlSubnetParser::buildSubnet(ParserData &data) {
  SubnetBuilder subnetBuilder;

  auto &nodes = data.nodes;
  auto &groups = data.groups;

  subnetBuilder.addInputs(groups[0].size());

  for (Node* node : data.groups[2]) {
    LinkList links;
    for (const Input &input : node->inputs) {
      links.emplace_back(nodes[input.id].id, input.inv);
    }
    node->id = subnetBuilder.addCellTree(model::AND, links, 2).idx;
  }

  for (Node* node : groups[1]) {
    const Input &input = node->inputs[0];
    node->id = subnetBuilder.addOutput(Link(nodes[input.id].id, input.inv)).idx;
  }

  return subnetBuilder.make();
}

} // namespace eda::gate::parser::graphml
