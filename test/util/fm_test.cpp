#include <iostream>
#include <fstream>

#include "gtest/gtest.h"

#include "util/fm.h"

void addEdge(std::vector<Vertex> &v, std::vector<MultiEdge> &e, std::ifstream &fin) {
    int vert_number;
    fin >> vert_number;
    e.emplace_back(MultiEdge{std::vector<int>(vert_number)});
    for (int &vertex: e.back().vertexes) {
        fin >> vertex;
        v[vertex].next.push_back(static_cast<int>(e.size()) - 1);
    }
}

void addEdge(std::vector<Vertex> &v, std::vector<MultiEdge> &e, std::vector<int> vertexes) {
    e.push_back(MultiEdge{std::move(vertexes)});
    for (int &vertex: e.back().vertexes) {
        v[vertex].next.push_back(static_cast<int>(e.size()) - 1);
    }
}

void addLinkedEdges(std::vector<Vertex> &v, std::vector<MultiEdge> &e, size_t step) {
    for (size_t i = 0; i < v.size(); i += (step - 1)) {
        std::vector<int> vertexes(step);
        for (size_t j = i; j < i + step && j < v.size(); ++j) {
            vertexes[j - i] = static_cast<int>(j);
        }
        addEdge(v, e, vertexes);
    }
}

void inputRndEdges(std::vector<Vertex> &v, std::vector<MultiEdge> &e, int edge_number, int edge_size) {
    e.resize(edge_number);
    std::vector<int> limits(edge_number);
    for (size_t i = 0; i < v.size(); ++i) {
        Vertex &vert = v[i];
        int edge = rand() % edge_number;
        if (limits[edge] >= edge_size) {
            edge = 0;
        }
        vert.next.push_back(edge);
        e[edge].vertexes.push_back(i);
        ++limits[edge];
    }
    for (size_t i = 0; i < limits.size(); ++i) {
        if (limits[i] == 0) {
            int ver = rand() % (v.size() - 1);
            e[i].vertexes.push_back(ver);
            e[i].vertexes.push_back(ver + 1);
            v[ver].next.push_back(i);
            v[ver + 1].next.push_back(i);
        } else if (limits[i] == 1) {
            int ver = (e[i].vertexes[0] + 1) % static_cast<int>(v.size());
            e[i].vertexes.push_back(ver);
            v[ver].next.push_back(i);
        }
    }
}

void inputRndWeights(std::vector<Vertex> &v) {
    for (size_t i = 0; i < v.size(); ++i) {
        v[i].weight = rand() % 100 + 1;
    }
}

void inputWeights(std::vector<Vertex> &v, std::ifstream &fin) {
    for (Vertex &vertex: v) {
        fin >> vertex.weight;
    }
}

void dotOutput(std::ofstream &fout, const std::vector<Vertex> &v, std::vector<MultiEdge> &e) {
    const char *colors[] = {"blue", "red"};
    fout << "graph partitioned {\n";
    for (int side = 0; side < 2; ++side) {
        fout << "\tsubgraph cluster_" << side << " {\n";
        for (size_t i = 0; i < v.size(); ++i) {
            if (v[i].side == side) {
                fout << "\t\tv" << i;
                // fout<< "[label=" << v[i].weight << "]";
                fout << ";\n";
            }
        }
        for (size_t i = 0; i < e.size(); ++i) {
            bool there = false;
            for (int v_number: e[i].vertexes) {
                there |= v[v_number].side != side;
            }
            if (!there) {
                fout << "\t\te" << i << "[shape=point];\n";
            }
        }
        fout << "\t\tcolor=" << colors[side] << ";\n";
        fout << "\t}\n";
    }
    for (size_t i = 0; i < e.size(); ++i) {
        fout << "\te" << i << "[shape=point];\n";
        for (int v_number: e[i].vertexes) {
            if (v_number & 1) {
                fout << "\te" << i << " -- v" << v_number << ";\n";
            } else {
                fout << "\tv" << v_number << " -- e" << i << ";\n";
            }
        }
    }
    fout << "}";
}

void graphOutput(const std::string &filename, const std::vector<Vertex> &v, std::vector<MultiEdge> &e) {
    std::ofstream fout(filename);
    if (fout.is_open()) {
        dotOutput(fout, v, e);
        fout.close();
    }
}

void print(const std::vector<Vertex> &v) {
    int area[2]{};
    for (int side = 0; side < 2; ++side) {
        std::cout << side << " : {";
        for (size_t i = 0; i < v.size(); ++i) {
            area[v[i].side] += v[i].weight;
            if (v[i].side == side) {
                std::cout << " " << i;
            }
        }
        std::cout << " } ";
        std::cout << area[side] << '\n';
    }
    std::cout << std::endl;
}

int countCutSet(const std::vector<MultiEdge> &e) {
    int cutset = 0;
    for (auto &edge: e) {
        cutset += (edge.distrib[0] != 0 && edge.distrib[1] != 0);
    }
    return cutset;
}

void testRandom(int seed, int passes, int vertex_number, int edge_number, int edge_size_limit, const std::string &output,
           double r) {
    srand(seed);
    std::vector<Vertex> v(vertex_number);
    std::vector<MultiEdge> e;
    inputRndWeights(v);
    inputRndEdges(v, e, edge_number, edge_size_limit);
    fm(v, e, r, passes);
    graphOutput(output, v, e);
}

void testLinked(int seed, int passes, int vertex_number, size_t step, const std::string &output, double r) {
    srand(seed);
    std::vector<Vertex> v(vertex_number);
    std::vector<MultiEdge> e;
    inputRndWeights(v);
    addLinkedEdges(v, e, step);
    fm(v, e, r, passes);
    graphOutput(output, v, e);
}

int testFMInput(int passes, const std::string &filename, const std::string &filename_out) {
    std::ifstream fin(filename);
    int cutset = -1;
    if (fin.is_open()) {
        int vertex_number, edge_number;
        fin >> vertex_number >> edge_number;
        std::vector<Vertex> v(vertex_number);
        std::vector<MultiEdge> e;
        inputWeights(v, fin);
        for (int i = 0; i < edge_number; ++i) {
            addEdge(v, e, fin);
        }
        double r;
        int power2;
        fin >> r >> power2;
        fm(v, e, r, passes);
        cutset = countCutSet(e);
        fin.close();
        // Outputs answer for debug.
        std::cout << "cutset: " << cutset << '\n';
        print(v);
        graphOutput(filename_out, v, e);
    }
    return cutset;
}

TEST(FMTest, BookTest) {
  EXPECT_EQ(testFMInput(10, "test/data/fm/test_book.txt", "test/data/fm/graph_test_book.txt"), 1);
}

TEST(FMTest, RandTest) {
  testRandom(123, 10000, 250, 200, 10, "test/data/fm/graph_rand_250.txt", 0.375);
}

TEST(FMTest, StructureGraphTest) {
  testLinked(123, 10000, 250, 30, "test/data/fm/graph_link_250.txt", 0.375);
}
