//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/printer/dot.h"

Dot::Dot(const Dot::GNet *gNet) : gNet(gNet) {}

void Dot::print(const std::string &filename) const {
  std::ofstream out(filename);
  if (out.is_open()) {
    print(out);
    out.close();
  } else {
    std::cerr << "Failed to create file : " << filename << std::endl;
  }
}

void Dot::print(std::ofstream &stream) const {
  stream << "digraph subNet {\n";
  for (const auto &gate: gNet->gates()) {
    if(gate->links().empty()) {
      stream << "\t";
      print(stream, gate);
      stream << ";\n";
    }
    for (const auto &links: gate->links()) {
      stream << "\t";
      print(stream, gate);
      stream << " -> ";
      print(stream, Gate::get(links.target));
      stream << ";\n";
    }
  }
  stream << "}" << std::endl;
}

void
Dot::print(std::ofstream &stream, const eda::gate::model::Gate *gate) const {
  stream << gate->id();
}
