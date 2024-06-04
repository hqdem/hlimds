//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/comb_mapper/func_mapper/delay_estmt/delay_estmt.h"
#include "gate/techmapper/comb_mapper/func_mapper/func_mapper.h"

#include <unordered_map>

namespace eda::gate::techmapper {
struct Gen {
  using SubnetID = model::SubnetID;

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

  void calculateFitness();
  float calculateChromosomeMaxArrivalTime();
  float findMaxArrivalTime(std::unordered_set<size_t> inputs);
};

class GeneticMapper : public FuncMapper {
  void map(const SubnetID subnetID,
           const CellDB &cellDB,
           const SDC &sdc,
           Mapping &mapping) override;

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

  void startEvolution(const SDC &sdc);

  void initialization(const SDC &sdc, const CellDB &cellDB);

  void reproduction();
  void mutation();
  void selection(const SDC &sdc);

  void hardSelection(const SDC &sdc);
  Chromosome createChild(const Chromosome& parent1, const Chromosome& parent2);
  int getRandomIndex(int min, int max, std::mt19937& gen);
  void saveBestChromosome();
  void rewriteCrossover(Chromosome &child,
                        const Chromosome &parent,
                        const std::shared_ptr<Gen> &parentGen);
  void fillChromosomeFromOutput(Chromosome& chromosome, size_t outputIndex,
                                const SDC &sdc);
  void saveInBestMap(Mapping &mapping);
  optimizer::CutExtractor *cutExtractor;
};
} // namespace eda::gate::techmapper
