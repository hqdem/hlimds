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

std::shared_ptr<Builder> GmlTranslator::translate(
    const std::string &file) const {
  ParserData data;
  return translate(file, data);
}

std::shared_ptr<Builder> GmlTranslator::translate(const std::string &file,
                                                  ParserData &data) const {
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

void GmlTranslator::parseGraph(XMLElement *graph, ParserData &data) const {
  XMLElement *element = findChild(graph);

  for (; checkName(element, "node"); element = next(element)) {
    parseNode(element, data);
  }

  for (; element; element = next(element)) {
    parseEdge(element, data);
  }
}

void GmlTranslator::parseNode(XMLElement *node, ParserData &data) const {
  uint32_t id = std::stoi(node->Attribute("id"));

  XMLElement *type = next(findChild(node));
  XMLElement *invIns = next(type);

  auto it = data.nodes.emplace(id, Node(getNum(type), getNum(invIns))).first;
  data.groups[getGroup(type)].push_back(&(it->second));
}

void GmlTranslator::parseEdge(XMLElement *edge, ParserData &data) const {
  Node &sourceNode = data.nodes.at(std::stoi(edge->Attribute("target")));
  Node &targetNode = data.nodes.at(std::stoi(edge->Attribute("source")));

  targetNode.inputs.push_back({&sourceNode, (bool)getNum(findChild(edge))});
}

std::shared_ptr<Builder> GmlTranslator::buildSubnet(ParserData &data) const {
  auto builder = std::make_shared<Builder>();
  auto &groups = data.groups;

  buildGroup(groups[0], builder.get());
  buildGroup(groups[2], builder.get());
  buildGroup(groups[1], builder.get());

  return builder;
}

void GmlTranslator::buildGroup(std::vector<Node*> &group,
                               Builder *builder) const {
  for (Node* node : group) {
    LinkList links;
    for (const Input &input : node->inputs) {
      uassert(input.node->link, "Input cells is not creared" << std::endl);
      const auto link = input.node->link.value();
      links.emplace_back(input.inv ? ~link : link);
    }
    const auto [symbol, inv] = typeMap.at(node->type);
    auto link = builder->addCell(symbol, links);
    node->link = inv ? ~link : link;
  }
}

} // namespace eda::gate::translator
