//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "util/fm.h"
#include "util/fm_hgraph.h"

#include "gtest/gtest.h"

#include <filesystem>

namespace fs = std::filesystem;

struct FMAlgoConfig {
  int seed;
  int passes;
  int weightLimit;
  int nodeNumber;
  int edgeNumber;
  int edgeSizeLimit;
  double r;
  size_t step;
};

//  Tests fm algorithm with a randomly created hypergraph.
void testRandom(const FMAlgoConfig &config, const std::string &outSubPath) {
  const fs::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::string outPath = homePath / outSubPath;
  HyperGraph graph(config.nodeNumber, config.seed);

  graph.setRndWeights(config.weightLimit);
  graph.setRndEdges(config.edgeNumber, config.edgeSizeLimit);

  FMAlgo<HyperGraph, int, int> algo(&graph, config.r, config.passes);

  algo.fm();
  if (!graph.graphOutput(outPath, algo.getSides())) {
    std::cerr << "Error opening or creating file: " << outPath << '\n';
  }
}

//  Tests fm algorithm with the hypergraph with pattern-created edges.
int testLinked(const FMAlgoConfig &config, const std::string &outSubPath) {
  const fs::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::string outPath = homePath / outSubPath;
  HyperGraph graph(config.nodeNumber, config.seed);

  graph.setRndWeights(config.weightLimit);
  graph.addLinkedEdges(config.step);

  FMAlgo<HyperGraph, int, int> algo(&graph, config.r, config.passes);

  algo.fm();
  if (!graph.graphOutput(outPath, algo.getSides())) {
    std::cerr << "Error opening or creating file: " << outPath << '\n';
  }
  return graph.countCutSet(algo.getDistrib());
}

//  Tests fm algorithm with the hypergraph from input file.
int testFMInput(int passes, const std::string &inSubPath,
                const std::string &outSubPath) {
  const fs::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::string inPath = homePath / inSubPath;
  const std::string outPath = homePath / outSubPath;
  std::ifstream fin(inPath);
  int cutset = -1;

  if (fin.is_open()) {
    int node_number, edge_number;
    fin >> node_number >> edge_number;
    HyperGraph graph(node_number);
    graph.setWeights(fin);
    for (int i = 0; i < edge_number; ++i) {
      graph.addEdge(fin);
    }
    double r;
    int power2;
    fin >> r >> power2;
    fin.close();
    FMAlgo<HyperGraph, int, int> algo(&graph, r, passes);
    algo.fm();
    cutset = graph.countCutSet(algo.getDistrib());
    if (!graph.graphOutput(outPath, algo.getSides())) {
      std::cerr << "Error opening or creating file: " << outPath << '\n';
    }
  }
  return cutset;
}

TEST(FMTest, BookFmTest) {
  const std::string pathIn = "test/data/fm/test_Kahng_in.txt";
  const std::string pathOut = "test/data/fm/test_Kahng_out1.txt";
  const std::string pathOut2 = "test/data/fm/test_Kahng_out2.txt";

  EXPECT_EQ(testFMInput(1, pathIn, pathOut), 2);
  EXPECT_EQ(testFMInput(2, pathIn, pathOut2), 1);
}

TEST(FMTest, RandFmTest) {
  FMAlgoConfig config;
  config.seed = 123;
  config.passes = 10000;
  config.weightLimit = 100;
  config.nodeNumber = 250;
  config.edgeNumber = 250;
  config.edgeSizeLimit = 10;
  config.r = 0.375;
  const std::string pathOut = "test/data/fm/graph_rand_250.txt";

  testRandom(config, pathOut);
}

TEST(FMTest, StructureFmGraphTest) {
  FMAlgoConfig config;
  config.seed = 123;
  config.passes = 10000;
  config.weightLimit = 100;
  config.nodeNumber = 250;
  config.step = 30;
  config.r = 0.375;
  const std::string pathOut = "test/data/fm/graph_link_250.txt";

  testLinked(config, pathOut);
}

TEST(FMTest, BigPartitionTest) {
  FMAlgoConfig config;
  config.seed = 123;
  config.passes = 10000;
  config.weightLimit = 100;
  config.nodeNumber = 100'000;
  config.step = 30;
  config.r = 0.375;
  const std::string out = "test/data/fm/graph_link_100000.txt";

  testLinked(config, out);
}

