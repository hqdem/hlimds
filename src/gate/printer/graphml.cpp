//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "graphml.h"

const std::string templatesFolderPath = "src/data/ctemplate/graphml/";

namespace eda::printer::graphMl {

std::map<std::string, std::string> toGraphMl::colours = {
  {"blue", "#CCCCFF"},
  {"green", "#34EB71"},
  {"red", "#EB3446"}
};

void toGraphMl::setShape(const GateSymbol &gate, 
        ctemplate::TemplateDictionary &dict) {
  switch (gate)
  {
  case GateSymbol::IN:
    dict.SetValue("NODE_SHAPE", "ellipse");
    break;
  case GateSymbol::OUT:
    dict.SetValue("NODE_SHAPE", "ellipse");
    break;
  case GateSymbol::ZERO:
    dict.SetValue("NODE_SHAPE", "rectangle");
    break;
  case GateSymbol::ONE:
    dict.SetValue("NODE_SHAPE", "rectangle");
    break;
  case GateSymbol::NOP:
    dict.SetValue("NODE_SHAPE", "triangle2");
    break;
  case GateSymbol::NOT:
    dict.SetValue("NODE_SHAPE", "triangle2");
    break;
  case GateSymbol::AND:
    dict.SetValue("CONFIG", "com.yworks.flowchart.delay");
    break;
  case GateSymbol::NAND:
    dict.SetValue("CONFIG", "com.yworks.flowchart.delay");
    break;
  case GateSymbol::OR:
    dict.SetValue("CONFIG", "com.yworks.flowchart.storedData");
    break;
  case GateSymbol::NOR:
    dict.SetValue("CONFIG", "com.yworks.flowchart.storedData");
    break;
  case GateSymbol::XOR:
    dict.SetValue("CONFIG", "com.yworks.flowchart.directData");
    break;
  case GateSymbol::XNOR:
    dict.SetValue("CONFIG", "com.yworks.flowchart.directData");
    break;
  default:
    dict.SetValue("NODE_SHAPE", "rectangle");
    break;
  }
}

const std::string toGraphMl::setGateSymbol(const GateSymbol &gate,
    const std::string colour) {
  std::string filename = templatesFolderPath + "shapenode.tpl";
  std::string output;
  ctemplate::TemplateDictionary dict("nodeGeometry");

  dict.SetValue("NODE_NAME", gate.name());
  dict.SetValue("NODE_COLOUR", colours[colour]);
  setShape(gate, dict);
  
  if (gate >= GateSymbol::AND && gate <= GateSymbol::XNOR) {
    filename = templatesFolderPath + "genericnode.tpl";
  }
  ctemplate::ExpandTemplate(filename, 
      ctemplate::DO_NOT_STRIP, 
      &dict, 
      &output);

  return output;
}

std::string toGraphMl::printNode(const Gate *node,
    const std::string colour) {
  std::string output;
  ctemplate::TemplateDictionary dict("nodetemplate");
  dict.SetIntValue("NODE_ID", node->id());
  dict.SetValue("NODE_GEOMETRY", setGateSymbol(node->func(), colour));

  ctemplate::ExpandTemplate(templatesFolderPath + "nodetemplate.tpl", 
      ctemplate::DO_NOT_STRIP, 
      &dict, 
      &output);

  return output;
}

std::string toGraphMl::printEdge(const Link &link, const bool sourceHasNegation) {
  std::string output;
  const std::string link_description = linkToString(link);
  ctemplate::TemplateDictionary dict("nodetemplate");

  dict.SetValue("EDGE_ID", link_description);
  dict.SetIntValue("SOURCE_ID", link.source);
  dict.SetIntValue("TARGET_ID", link.target);
  dict.SetIntValue("INPUT", link.input);

  // If source has negation it draws white circle as source arrow which
  // imitates negation symbol
  dict.SetValue("SRC_ARROW", sourceHasNegation ? "white_circle" : "none");
  ctemplate::ExpandTemplate(templatesFolderPath + "edgetemplate.tpl", 
      ctemplate::DO_NOT_STRIP, 
      &dict, 
      &output);

  return output;
}

const std::string toGraphMl::linkToString (const Link &link) {
  return std::to_string(link.source) + "_" + std::to_string(link.target) 
  + "_" + std::to_string(link.input);
}

void toGraphMl::printer (std::ostream &output, const GNet &model) {
  std::string nodeOutput;
  std::string edgeOutput;
  std::string docOutput;

  // Document header
  ctemplate::TemplateDictionary dict("doctemplate");

  dict.SetIntValue("GRAPH_ID", model.id());

  const auto &allGates = model.gates();
  for (auto *const gate: allGates) {
    // Output a description of the nodes of the graph and maks it blue
    nodeOutput += printNode(gate, "blue");
    const auto &allLinksFromGate = gate->links();
    const bool negationFlag = gate->isNegation();
    for (const auto link: allLinksFromGate) {
      // Check whether this node is the beginning for the edge
      if (link.source == gate->id()) {
        edgeOutput += printEdge(link, negationFlag);
        // If the target node isn't in the graph, 
        // then draw it and mark it red
        if (!model.hasNode(link.target)) {
          nodeOutput += printNode(model.gate(link.target), "red");
        }
      }
      else {
        // If the source node isn't in the graph, 
        // then draw it and mark it in green
        if (!model.hasNode(link.source)) {
          nodeOutput += printNode(model.gate(link.source), "green");
          edgeOutput += printEdge(link, negationFlag);
        }
      }
    }
  }
  dict.SetValue("NODE_DATA", nodeOutput);
  dict.SetValue("EDGE_DATA", edgeOutput);
  // End of document
  ctemplate::ExpandTemplate(templatesFolderPath + "doctemplate.tpl", 
      ctemplate::DO_NOT_STRIP, 
      &dict, 
      &docOutput);
  
  output << docOutput;
}

}; //namespace eda::printer::graphMl
