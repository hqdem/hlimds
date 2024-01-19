//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml.h"

using namespace tinyxml2;

namespace eda::gate::parser::graphml {

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

  model::GNet *GraphMLParser::parse(const std::string &filename) {
    ParserData data;
    return parse(filename, data);
  }

  model::GNet *GraphMLParser::parse(const std::string &filename,
                                    GraphMLParser::ParserData &data) {
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
      LOG_ERROR << "Error loading file: " << filename << std::endl;
      return nullptr;
    }

    XMLElement *root = doc.RootElement();
    if (!root) {
      LOG_ERROR << "No root element found in file: " << filename << std::endl;
      return nullptr;
    }

    XMLElement *graphNode = findGraph(root);
    if (graphNode) {
      iterateFromGraphNode(graphNode, data);
    }

    linkNet(data);

    return data.gnet;
  }

  void GraphMLParser::iterateFromGraphNode(XMLElement *graphNode,
                                           ParserData &data) {
    XMLElement *element = graphNode->FirstChildElement();
    while (element) {
      const char *tagName = element->Value();

      if (tagName && !std::strcmp(tagName, "node")) {
        parseNode(element, data);
      } else if (tagName && !std::strcmp(tagName, "edge")) {
        parseEdge(element, data);
      }
      element = element->NextSiblingElement();
    }
  }

  void GraphMLParser::parseNode(XMLElement *nodeElement, ParserData &data) {
    auto nodeId = std::stoi(nodeElement->Attribute("id"));

    auto &gateData = data.gates[nodeId];
    gateData.id = data.gnet->newGate();

    XMLElement *dataElement = nodeElement->FirstChildElement("data");
    while (dataElement) {
      const char *value = dataElement->GetText();

      if (dataElement->Attribute("key", "d1")) {
        if (!std::strcmp(value, "0")) {
          gateData.kind = GateSymbol::IN;
        } else if (!std::strcmp(value, "1")) {
          gateData.kind = GateSymbol::OUT;
        } else {
          gateData.kind = GateSymbol::AND;
        }
      } else if (dataElement->Attribute("key", "d2")) {
        gateData.invertedNumber = std::stoi(value);
      }

      dataElement = dataElement->NextSiblingElement("data");
    }
  }

  void GraphMLParser::parseEdge(XMLElement *edgeElement, ParserData &data) {
    int sourceId = std::stoi(edgeElement->Attribute("target"));
    int targetId = std::stoi(edgeElement->Attribute("source"));

    auto &gateData = data.gates[targetId];

    XMLElement *dataElement = edgeElement->FirstChildElement("data");
    while (dataElement) {
      const char *value = dataElement->GetText();

      if (dataElement->Attribute("key", "d3")) {
        bool inv = std::strcmp(value, "0");
        gateData.inputs.emplace_back(InputData{sourceId, inv});

        dataElement = dataElement->NextSiblingElement("data");
      }
    }
  }

  void GraphMLParser::linkNet(ParserData &data) {
    for (const auto &[id, gate]: data.gates) {

      std::vector<base::model::Signal<GateId>> inputs;
      int invInputCounter = 0;

      for (const auto &input: gate.inputs) {
        GateId inputId;
        if (input.inverted) {
          ++invInputCounter;
          inputId = data.gnet->addNot(data.gates[input.input].id);
        } else {
          inputId = data.gates[input.input].id;
        }
        inputs.emplace_back(base::model::ALWAYS, inputId);
      }
      data.gnet->setGate(gate.id, gate.kind, inputs);

      assert(gate.invertedNumber == invInputCounter &&
             "Parser error - Incorrect building of NOT gates.");
    }
  }
} // namespace eda::gate::parser::graphml