//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gtest/gtest.h"

#include "fm_graph.h"
#include "util/fm.h"

void testRandom(int seed, int passes, int vertexNumber, int edgeNumber,
                int edgeSizeLimit, const std::string &output,
                double r) {
    Graph graph(vertexNumber, seed);
    graph.inputRndWeights();
    graph.inputRndEdges(edgeNumber, edgeSizeLimit);
    FMAlgo<Graph, int, int> algo(&graph, r, 3);
    algo.fm();
    graph.graphOutput(output, algo.getSides());
}

int testLinked(int seed, int passes, int vertexNumber, size_t step,
               const std::string &output, double r) {
    Graph graph(vertexNumber, seed);
    graph.inputRndWeights();
    graph.addLinkedEdges(step);
    FMAlgo<Graph, int, int> algo(&graph, r, 3);
    algo.fm();
    graph.graphOutput(output, algo.getSides());
    return graph.countCutSet(algo.getDistrib());
}

int testFMInput(int passes, const std::string &filename,
                const std::string &filename_out) {
    std::ifstream fin(filename);
    int cutset = -1;
    if (fin.is_open()) {
        int vertex_number, edge_number;
        fin >> vertex_number >> edge_number;
        Graph graph(vertex_number);
        graph.inputWeights(fin);
        for (int i = 0; i < edge_number; ++i) {
            graph.addEdge(fin);
        }
        double r;
        int power2;
        fin >> r >> power2;
        fin.close();
        FMAlgo<Graph, int, int> algo(&graph, r, 3);
        algo.fm();
        cutset = graph.countCutSet(algo.getDistrib());
        graph.graphOutput(filename_out, algo.getSides());
    }
    return cutset;
}

TEST(FMTest, BookTest) {
    EXPECT_EQ(testFMInput(1, "test/data/fm/test_book.txt",
                          "test/data/fm/graph_test_book2.txt"),2);
}

TEST(FMTest, RandTest) {
    testRandom(123, 10000, 250, 200, 10,
               "test/data/fm/graph_rand_250.txt", 0.375);
}

TEST(FMTest, StructureGraphTest) {
    testLinked(123, 10000, 250, 30,
               "test/data/fm/graph_link_250.txt", 0.375);
}
