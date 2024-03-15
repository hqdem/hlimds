//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/techoptimizer/mapper/cut_base/cut_base_mapper.h"

#include <unordered_map>

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */

using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {
struct Gen{
  bool emptyGen = true;
  bool isIn = false;
  bool isOut = false;
  SubnetID subnetID;
  std::string name;
  float area = 0;
  std::unordered_set<size_t> entryIdxs;
};

struct Chromosome{
  std::vector<std::shared_ptr<Gen>> gens;
  float area = 0;
  float arrivalTime = 1;

  // 1 / (area * arrivalTime)
  float fitness;

  void calculateFitness();
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
  int nParents = 500;
  int nPairs = 500;
  int nChild = 500;

  int nGenerations = 1000;

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
} // namespace eda::gate::tech_optimizer