//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/comb_mapper/cut_based/cut_based_mapper.h"
#include "gate/techmapper/comb_mapper/cut_based/delay_estmt/delay_estmt.h"

#include <unordered_map>

namespace eda::gate::techmapper {
struct Gen {
  using SubnetID = eda::gate::model::SubnetID;

  bool emptyGen = true;
  bool isIn = false;
  bool isOut = false;

  SubnetID subnetID;
  std::string name;

  float area = 0;
  float arrivalTime = 0;

  std::unordered_set<size_t> entryIdxs;
};

struct Chromosome {
  std::vector<std::shared_ptr<Gen>> gens;
  float area = 0;
  float arrivalTime = 0;

  // 1 / (area * arrivalTime)
  float fitness;

  void calculateFitness(Library &lib);
  float calculateChromosomeMaxArrivalTime(Library &lib);
  float findMaxArrivalTime(std::unordered_set<size_t> inputs);
};

class GeneticMapper : public CutBaseMapper {
protected:
  void findBest() override;

private:
  //std::vector<Chromosome> basePopulation;

  std::vector<Chromosome> parentChromosomes;
  std::vector<Chromosome> nextGeneration;

  Chromosome bestChromosome;

  std::vector<std::vector<std::shared_ptr<Gen>>> genBank;

  int nBasePopulation = 1000;
  int nParents = 250;
  int nPairs = 250;
  long unsigned int nChild = 1000;

  int nGenerations = 50;

  Library lib;

  void startEvolution();

  void initialization();

  void reproduction();
  void mutation();
  void selection();

  void hardSelection();
  Chromosome createChild(const Chromosome& parent1, const Chromosome& parent2);
  int getRandomIndex(int min, int max, std::mt19937& gen);
  void saveBestChromosome();
  void rewriteCrossover(Chromosome &child,
                        const Chromosome &parent,
                        const std::shared_ptr<Gen> &parentGen);
  void fillChromosomeFromOutput(Chromosome& chromosome, size_t outputIndex);
  void saveInBestMap();
};
} // namespace eda::gate::techmapper
