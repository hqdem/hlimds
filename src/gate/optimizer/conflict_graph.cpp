//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "conflict_graph.h"

#include <algorithm>
#include <ostream>
#include <set>
#include <stack>

namespace eda::gate::optimizer { 

float ConflictGraph::coloring(SubnetBuilder *builder) {      
                 
  std::stack<Vertice*> st;
  st.push(graph[0]);
  float maxDelta = 0;

  while (!st.empty()) {

    Vertice *v = st.top(); 
    st.pop();

    bool visit = false;
    for (auto cell : v->numCells) {
      if (this->isVisited[cell]) {
        visit = true;
      }
    }
    if (visit) {
      continue;
    }

    for (auto cell : v->numCells) {
      this->isVisited[cell] = true;  
    }

    maxDelta += v->delta;
    builder->replace(v->subnetID, v->entryMap);

    std::pop_heap(graph.begin(), graph.end());
    delete graph[graph.size() - 1];
    graph.pop_back();
    
    if (graph.size()) {
      st.push(graph[0]);
    }
  }
  return maxDelta;
}

float ConflictGraph::findBestColoring(SubnetBuilder *builder) {

  std::make_heap(graph.begin(), graph.end(), 
      [] (auto lhs, auto rhs) { return lhs->delta < rhs->delta; } );
  
  float maxDelta = 0;
  std::stack<Vertice*> st;
  this->isVisited.resize(builder->getMaxIdx(), false);

  while (graph.size()) {

    bool visit = false;
    for (size_t cell : graph[0]->numCells) {
      if (this->isVisited[cell]) {
        visit = true;
      }
    }

    if (visit) {
      std::pop_heap(graph.begin(), graph.end());
      delete graph[graph.size() - 1];
      graph.pop_back();
    } else {
      maxDelta += coloring(builder);
    }
  }

  return maxDelta;
}

} // namespace eda::gate::optimizer
