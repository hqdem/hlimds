//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include <assert.h>

#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/optimizer2/cone_builder.h"
#include "gate/techmapper/mapper/cut_base/genetic/genetic_mapper.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <filesystem>
#include <numeric>
#include <random>
#include <vector>

namespace eda::gate::tech_optimizer {
void GeneticMapper::findBest() {
  std::string file_name = "test/data/gate/tech_mapper/sky130_fd_sc_hd__ff_100C_1v65.lib";

  const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
  const std::filesystem::path filePath = homePath / file_name;

  TokenParser tokParser;
  FILE *file = fopen(filePath.generic_string().c_str(), "rb");
  Group *ast = tokParser.parseLibrary(file,
                                      filePath.generic_string().c_str());
  AstParser parser(lib, tokParser);
  parser.run(*ast);
  fclose(file);

  initialization();
  startEvolution();
  saveInBestMap();
}

void GeneticMapper::initialization() {
  optimizer2::ConeBuilder coneBuilder(&model::Subnet::get(subnetID));
  auto entries = model::Subnet::get(subnetID).getEntries();
  genBank.resize(std::size(entries));

  for (uint64_t entryIndex = 0; entryIndex < std::size(entries);
       entryIndex++) {
    if (entries[entryIndex].cell.isIn()) {
      auto gen = std::make_shared<Gen>();
      gen->emptyGen = false;
      gen->isIn = true;
      gen->name = "IN";
      genBank[entryIndex].push_back(gen);
      continue;
    }
    if (entries[entryIndex].cell.isOut()) {
      auto gen = std::make_shared<Gen>();
      gen->emptyGen = false;
      gen->isIn = false;
      gen->isOut = true;
      gen->name = "OUT";
      gen->entryIdxs.insert(entries[entryIndex].cell.link[0].idx);
      genBank[entryIndex].push_back(gen);
      continue;
    } else {
      auto cutsList = cutExtractor->getCuts(entryIndex);
      for (const auto &cut: cutsList) {
        if (cut.entryIdxs.size() != 1) {

          SubnetID coneSubnetID = coneBuilder.getCone(cut).subnetID;
          auto truthTable = eda::gate::model::evaluate(
              model::Subnet::get(coneSubnetID));

          for (const SubnetID &currentSubnetID: cellDB->getSubnetIDsByTT(
              truthTable.at(0))) {
            //fill gen bank with currentSubnetID and currentAttr
            auto currentAttr = cellDB->getSubnetAttrBySubnetID(currentSubnetID);
            auto gen = std::make_shared<Gen>();
            gen->emptyGen = false;
            gen->subnetID = currentSubnetID;
            gen->name = currentAttr.name;
            gen->area = currentAttr.area;
            gen->entryIdxs = cut.entryIdxs;

            genBank[entryIndex].push_back(gen);
          }
        }
      }
    }
  }


  // and now we fill std::vector<Chromosome> nextGeneration with random chromosome
  nextGeneration.clear();

  std::vector<size_t> outputGenIndexes;
  for (size_t i = 0; i < genBank.size(); ++i) {
      if (genBank[i][0]->isOut) {
        outputGenIndexes.push_back(i);
        break;
    }
  }

  for (int i = 0; i < nBasePopulation; ++i) {
    Chromosome newChromosome;
    newChromosome.gens.resize(genBank.size());

    for (const auto& index : outputGenIndexes) {
      fillChromosomeFromOutput(newChromosome, index);
    }

    for (long unsigned int j = 0; j < genBank.size(); j++) {
      if (newChromosome.gens[j] == nullptr) {
        auto gen = std::make_shared<Gen>();
        gen->emptyGen = true;
        gen->name = "Empty";
        newChromosome.gens[j] = gen;
      }
    }
    newChromosome.calculateFitness(lib);

    nextGeneration.push_back(newChromosome);
  }

}

void GeneticMapper::fillChromosomeFromOutput(Chromosome& chromosome, size_t outputIndex) {
  if (chromosome.gens[outputIndex] != nullptr) return;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, genBank[outputIndex].size() - 1);
  size_t genIndex = distrib(gen);
  std::shared_ptr<Gen> selectedGen = genBank[outputIndex][genIndex];
  if (!selectedGen->isIn) {
    for (const auto &entryIdx: selectedGen->entryIdxs) {
      fillChromosomeFromOutput(chromosome, entryIdx);
    }
  }
  chromosome.gens[outputIndex] = selectedGen;
}


void GeneticMapper::startEvolution() {
  auto startTime = std::chrono::steady_clock::now();
  auto endTime = startTime + std::chrono::minutes(1);

  for (int generation = 0; generation < nGenerations; ++generation) {
    auto currentTime = std::chrono::steady_clock::now();
    if (currentTime >= endTime) {
      std::cout << "Time limit reached. Ending evolution." << std::endl;
      break; // Exit the loop if the time limit is reached.
    }

    selection();
    reproduction();
    saveBestChromosome();
    mutation();

    std::cout << "Generation " << generation + 1 << " completed." << std::endl;
  }
  std::cout << "Evolution completed." << std::endl;
}

// Auxiliary function for roulette wheel selection
void GeneticMapper::selection() {
  hardSelection();
  // Calculating the total fitness for the entire population
  float totalFitness = std::accumulate(nextGeneration.begin(),
                                       nextGeneration.end(),
                                       0.0f,
                                       [](float sum, const Chromosome& c) { return sum + c.fitness; });

  // Creating a new vector for parent chromosomes
  std::vector<Chromosome> selectedParents;
  selectedParents.reserve(nParents);

  // Generating random numbers for roulette wheel selection
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0, totalFitness);

  for (int i = 0; i < nParents; ++i) {
    // Random value for selecting a parent
    float randValue = dis(gen);
    float sum = 0;

    // Searching for a parent matching the random value
    for (const auto& chromosome : nextGeneration) {
      sum += chromosome.fitness;
      if (sum >= randValue) {
        selectedParents.push_back(chromosome);
        break; // Breaking the loop, parent found
      }
    }
  }

  // Replacing current parent chromosomes with selected ones
  parentChromosomes = std::move(selectedParents);
}

void GeneticMapper::hardSelection() {
  // Using the erase-remove idiom to remove chromosomes that do not meet the criteria
  nextGeneration.erase(std::remove_if(nextGeneration.begin(), nextGeneration.end(),
                                      [this](const Chromosome& chromosome) {
                                        return chromosome.area > sdc.area || chromosome.arrivalTime > sdc.arrivalTime;
                                      }),
                       nextGeneration.end());
}

void GeneticMapper::reproduction() {
  std::vector<std::pair<Chromosome, Chromosome>> parentPairs;

  parentPairs.reserve(nPairs);

  // Make sure we have a non-deterministic random generator
  std::random_device rd;
  std::mt19937 gen(rd());

  for (int i = 0; i < nPairs; ++i) {
    if (parentChromosomes.size() < 2) {
      std::cerr << "Not enough parents to form a pair." << std::endl;
      break;
    }

    int parent1Index = getRandomIndex(0, parentChromosomes.size() - 1, gen);
    int parent2Index = getRandomIndex(0, parentChromosomes.size() - 1, gen);

    // Ensure parent2 is not the same as parent1
    while (parent1Index == parent2Index) {
      parent2Index = getRandomIndex(0, parentChromosomes.size() - 1, gen);
    }

    // Add the selected pair to parentPairs
    parentPairs.push_back({parentChromosomes[parent1Index], parentChromosomes[parent2Index]});
  }

  // Calculate total fitness of all pairs for proportional distribution of offspring
  float totalPairFitness = 0.0f;
  for (const auto& pair : parentPairs) {
    totalPairFitness += pair.first.fitness + pair.second.fitness;
  }

  // Clear the nextGeneration vector to populate it with new offspring
  nextGeneration.clear();

  // Generate offspring
  for (const auto& pair : parentPairs) {
    // Determine the number of children this pair should produce
    float pairFitness = pair.first.fitness + pair.second.fitness;
    int childrenForPair = static_cast<int>(std::round((pairFitness / totalPairFitness) * nChild));

    for (int i = 0; i < childrenForPair; ++i) {
      if (nextGeneration.size() < nChild) {
        Chromosome child = createChild(pair.first, pair.second);
        nextGeneration.push_back(child);
      } else {
        // Break early if we've already created the desired number of children
        break;
      }
    }
  }
}

Chromosome GeneticMapper::createChild(const Chromosome &parent1, const Chromosome &parent2) {
  Chromosome child;
  size_t totalGenes = parent1.gens.size(); // Assuming both parents have the same number of genes for simplicity

  // Randomly decide the crossover point
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(1, totalGenes - 1); // Crossover point; avoiding 0 to ensure a mix
  size_t crossoverPoint = distrib(gen);

  // Copy genes from parent1 up to the crossover point
  for (size_t i = 0; i < crossoverPoint; ++i) {
    child.gens.push_back(parent1.gens[i]);
  }

  for (size_t i = crossoverPoint; i < totalGenes; ++i) {
    auto addGen = parent2.gens[i];
    rewriteCrossover(child, parent2, parent2.gens[i]);
    child.gens.push_back(parent2.gens[i]);
  }

  child.calculateFitness(lib);
  return child;
}

void GeneticMapper::rewriteCrossover(Chromosome &child,
                                     const Chromosome &parent,
                                     const std::shared_ptr<Gen> &parentGen) {
  for (const auto &genIn : parentGen->entryIdxs) {
    if (child.gens.at(genIn)->emptyGen) {
      child.gens.at(genIn) = parent.gens.at(genIn);
      rewriteCrossover(child, parent, parent.gens.at(genIn));
    }
  }
}

void GeneticMapper::saveBestChromosome() {
  if (nextGeneration.empty()) {
    std::cerr << "Warning: nextGeneration is empty, cannot save the best chromosome." << std::endl;
    return;
  }

  // Initialize with the first chromosome as a simple starting point.
  Chromosome* best = &nextGeneration[0];

  // Iterate through the rest of the chromosomes to find the best one.
  for (auto& chromosome : nextGeneration) {
    if (chromosome.fitness > best->fitness) {
      best = &chromosome;
    }
  }

  // Save the best chromosome found.
  bestChromosome = *best;
}

int GeneticMapper::getRandomIndex(int min, int max, std::mt19937& gen) {
  std::uniform_int_distribution<> distr(min, max);
  return distr(gen);
}

void GeneticMapper::mutation() {

}

void GeneticMapper::saveInBestMap() {
  for (uint64_t entryIndex = 0; entryIndex < bestChromosome.gens.size();
       entryIndex++) {
    BestReplacement replacement;
    auto bestGen = bestChromosome.gens.at(entryIndex);
    replacement.isIN = bestGen->isIn;
    replacement.isOUT = bestGen->isOut;
    replacement.subnetID = bestGen->subnetID;
    replacement.entryIDxs = bestGen->entryIdxs;
    (*bestReplacementMap)[entryIndex] = replacement;
  }
}

void Chromosome::calculateFitness(Library &lib) {
  for (const auto &gen: gens) {
    area += gen->area;
  }
  arrivalTime = calculateChromosomeMaxArrivalTime(lib);
  fitness = 1 / area * arrivalTime;
}

float Chromosome::calculateChromosomeMaxArrivalTime(Library &lib) {
  float maxArrivalTime = 0;
  for (auto& gen : gens) {
    if (!gen->emptyGen) {
      if (gen->isIn || gen->isOut) {
        continue;
      } else {
        delay_estimation::DelayEstimator d1;
        float inputNetTransition = findMaxArrivalTime(gen->entryIdxs);
        size_t nIn = 1;
        float fanoutCap = d1.wlm.getFanoutCap(nIn) +
                          d1.nldm.getCellCap();

        d1.nldm.delayEstimation(gen->name,
                                lib,
                                inputNetTransition,
                                fanoutCap);

        gen->arrivalTime = d1.nldm.getSlew();
        (maxArrivalTime < gen->arrivalTime) ? maxArrivalTime = gen->arrivalTime : 0;
      }
    }
  }
  return maxArrivalTime;
}

float Chromosome::findMaxArrivalTime(std::unordered_set<size_t> inputs) {
  float maxArrivalTime = 0;
  for (const auto &in : inputs) {
    (gens[in]->arrivalTime > maxArrivalTime) ? maxArrivalTime = gens[in]->arrivalTime : 0;
  }
  return maxArrivalTime;
}


} // namespace eda::gate::tech_optimizer