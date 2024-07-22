//===----------------------------------------------------------------------===
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===

#include "graphml.h"
#include "util/assert.h"

#include <iostream>

namespace eda::gate::translator {

using tinyxml2::XMLElement;
using tinyxml2::XMLDocument;

using Builder    = GmlTranslator::Builder;
using ParserData = GmlTranslator::ParserData;

XMLElement *findGraph(XMLElement *root) {
  XMLElement *child = root->FirstChildElement();
  while (child) {
    const char *tagName = child->Value();
    if (tagName && !std::strcmp(tagName, "graph")) {
      return child;
    }
    child = child->NextSiblingElement();
  }
  uassert(false, "Graph Node is not found" << std::endl);
  return nullptr;
}

std::shared_ptr<Builder> GmlTranslator::translate(const std::string &file) {
  ParserData data;
  return translate(file, data);
}

std::shared_ptr<Builder> GmlTranslator::translate(const std::string &file,
                                                  ParserData &data) {
  XMLDocument doc;
  doc.LoadFile(file.c_str());
  uassert(!doc.ErrorID(), "Error loading file" << std::endl);

  XMLElement *root = doc.RootElement();
  uassert(root, "No root element found in file" << std::endl);

  XMLElement *graph = findGraph(root);
  uassert(graph, "No graph element found in file" << std::endl);

  parseGraph(graph, data);

  return buildSubnet(data);
}

void GmlTranslator::parseGraph(XMLElement *graph, ParserData &data) {
  XMLElement *element = findChild(graph);

  for (; checkName(element, "node"); element = next(element)) {
    parseNode(element, data);
  }

  for (; element; element = next(element)) {
    parseEdge(element, data);
  }
}

void GmlTranslator::parseNode(XMLElement *node, ParserData &data) {
  uint32_t id = std::stoi(node->Attribute("id"));

  XMLElement *type = next(findChild(node));
  XMLElement *invIns = next(type);

  data.nodes.emplace_back(id, getNum(type), getNum(invIns));
  data.groups[getNum(type)].push_back(&data.nodes.back());
}

void GmlTranslator::parseEdge(XMLElement *edge, ParserData &data) {
  Node &sourceNode = data.nodes.at(std::stoi(edge->Attribute("target")));
  Node &targetNode = data.nodes.at(std::stoi(edge->Attribute("source")));

  targetNode.inputs.push_back({&sourceNode, (bool)getNum(findChild(edge))});
}

std::shared_ptr<Builder> GmlTranslator::buildSubnet(ParserData &data) {
  auto builder = std::make_shared<Builder>();
  auto &groups = data.groups;

  builder->addInputs(groups[0].size());
  for (Node* node : data.groups[2]) {
    LinkList links;
    for (const Input &input : node->inputs) {
      links.emplace_back(input.node->id, input.inv);
    }
    node->id = builder->addCellTree(model::AND, links, 2).idx;
  }

  for (Node* node : groups[1]) {
    const Input &input = node->inputs[0];
    node->id = builder->addOutput(Link(input.node->id, input.inv)).idx;
  }

  return builder;
}

} // namespace eda::gate::translator
