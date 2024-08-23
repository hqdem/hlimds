//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnet.h"
#include "gate/optimizer/npn.h"
#include "gate/optimizer/npndb.h"
#include "util/env.h"

#include <percy/percy.hpp>

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

using Link = eda::gate::model::Subnet::Link;
using SubnetObject = eda::gate::model::SubnetObject;

//===----------------------------------------------------------------------===//
// AIG/XAG synthesis
//===----------------------------------------------------------------------===//

inline SubnetObject makeXagSubnet(uint8_t k, const percy::chain &chain) {
  const size_t nIn = chain.get_nr_inputs();
  const size_t nStep = chain.get_nr_steps();
  const size_t nOut = chain.get_nr_outputs();

  assert(k >= nIn);

  SubnetObject subnetObject;
  auto &subnetBuilder = subnetObject.builder();

  std::vector<Link> links(nIn + nStep);

  size_t j = 0;

  for (size_t i = 0; i < k; ++i) {
    const auto link = subnetBuilder.addInput();
    if (i < nIn) {
      links[j++] = link;
    }
  }

  for (size_t i = 0; i < nStep; ++i) {
    uint64_t table = 0;
    for (size_t j = 0; j < k; ++j) {
      if (kitty::get_bit(chain.get_operator(i), j)) {
        table |= (1 << j);
      }
    }    
    assert(table <= 0xf);

    const auto &args = chain.get_step(i);
    assert(args.size() == 2);

    switch (table) {
    case 0x2:
      links[j++] = subnetBuilder.addCell(
          eda::gate::model::AND, links[args[0]], ~links[args[1]]);
      break;
    case 0x4:
      links[j++] = subnetBuilder.addCell(
          eda::gate::model::AND, ~links[args[0]], links[args[1]]);
      break;
    case 0x6:
      links[j++] = subnetBuilder.addCell(
          eda::gate::model::XOR, links[args[0]], links[args[1]]);
      break;
    case 0x8:
      links[j++] = subnetBuilder.addCell(
          eda::gate::model::AND, links[args[0]], links[args[1]]);
      break;
    case 0xe:
      links[j++] = ~subnetBuilder.addCell(
          eda::gate::model::AND, ~links[args[0]], ~links[args[1]]);
      break;
    default:
      assert(false);
    }
  }

  for (size_t i = 0; i < nOut; ++i) {
    const auto lit = chain.get_outputs()[i];
    const auto inv = lit & 1;
    const auto var = lit >> 1;

    const auto link = (var == 0)
        ? subnetBuilder.addCell(eda::gate::model::ZERO)
        : links[var - 1];

    subnetBuilder.addOutput(inv ? ~link : link);
  }

  return subnetObject;
}

inline SubnetObject synthesizeXag(uint8_t k, percy::spec &spec) {
  percy::chain chain;
  const auto result = synthesize(spec, chain);
  assert(result == percy::success);

  return makeXagSubnet(k, chain);
}

inline SubnetObject synthesizeAig(uint8_t k, percy::spec &spec) {
  spec.set_primitive(percy::AIG);
  return synthesizeXag(k, spec);
}

//===----------------------------------------------------------------------===//
// MIG synthesis
//===----------------------------------------------------------------------===//

inline SubnetObject makeMigSubnet(uint8_t k, const percy::mig &chain) {
  const size_t nIn = chain.get_nr_inputs();
  const size_t nStep = chain.get_nr_steps();
  const size_t nOut = chain.outputs.size();

  eda::gate::model::SubnetObject subnetObject;
  auto &subnetBuilder = subnetObject.builder();

  std::vector<Link> links(1 + nIn + nStep);

  size_t j = 0;
  links[j++] = Link(0, 0);
  for (size_t i = 0; i < k; ++i) {
    const auto link = subnetBuilder.addInput();
    if (i < nIn) {
      links[j++] = link;
    }
  }
  for (const auto &args : chain.steps) {
    assert(args.size() == 3);
    if (!args[0] || !args[1] || !args[2]) {
      links[0] = subnetBuilder.addCell(eda::gate::model::ZERO);
      break;
    }
  }

  for (size_t i = 0; i < nStep; ++i) {
    const auto op = chain.operators[i];
    const auto &args = chain.steps[i];
    assert(args.size() == 3);

    switch (op) {
      case 0: links[j++] = subnetBuilder.addCell(eda::gate::model::MAJ,
          links[args[0]], links[args[1]], links[args[2]]);
          break;
      case 1: links[j++] = subnetBuilder.addCell(eda::gate::model::MAJ,
          ~links[args[0]], links[args[1]], links[args[2]]);
          break;
      case 2: links[j++] = subnetBuilder.addCell(eda::gate::model::MAJ,
          links[args[0]], ~links[args[1]], links[args[2]]);
          break;
      case 3: links[j++] = subnetBuilder.addCell(eda::gate::model::MAJ,
          links[args[0]], links[args[1]], ~links[args[2]]);
          break;
      default: assert(false);
    }
  }

  for (size_t i = 0; i < nOut; ++i) {
    const auto lit = chain.outputs[i];
    const auto inv = lit & 1;
    const auto var = lit >> 1;
    
    const auto link = (var == 0)
        ? subnetBuilder.addCell(eda::gate::model::ZERO)
        : links[var];

    subnetBuilder.addOutput(inv ? ~link : link);
  }

  return subnetObject;
}

inline SubnetObject synthesizeMig(uint8_t k, percy::spec &spec) {
  percy::mig chain;
  percy::bsat_wrapper solver;
  percy::mig_encoder encoder(solver);
  const auto result = mig_synthesize(spec, chain, solver, encoder);
  assert(result == percy::success);

  return makeMigSubnet(k, chain);
}

//===----------------------------------------------------------------------===//
// General functions
//===----------------------------------------------------------------------===//

inline void printBases() {
  std::cout << "Available bases: [aig, xag, mig]" << std::endl;
}

inline void printUsage() {
  std::cout << "Usage: dbgen [BASIS] [FILE]" << std::endl;
  printBases();
  std::cout << "With no FILE write to 'UTOPIA_HOME/output/db'" << std::endl;
  std::cout << std::endl << "Example: ./dbgen xag" << std::endl;
}

inline std::string toHexString(uint8_t k, uint64_t value) {
  assert(k <= 6);
  assert(value <= (1ull << (1 << k)) - 1);

  std::stringstream ss;
  ss << std::setfill('0') << std::setw(k) << std::hex << value;
  return ss.str();
}

inline SubnetObject synthesize(uint8_t k,
                               const std::string &tt,
                               const std::string &basis) {

  kitty::dynamic_truth_table func(k);
  kitty::create_from_hex_string(func, tt);

  percy::spec spec;
  spec.set_nr_out(1);
  spec[0] = func;

  if (basis == "aig") {
    return synthesizeAig(k, spec);
  }
  if (basis == "xag") {
    return synthesizeXag(k, spec);
  }
  if (basis == "mig") {
    return synthesizeMig(k, spec);
  }

  std::cout << "Error: unsupported basis for generation" << std::endl;
  printBases();
  return SubnetObject(eda::gate::model::OBJ_NULL_ID);
}

inline SubnetObject synthesize(uint8_t k,
                               uint64_t value,
                               const std::string &basis) {
  return synthesize(k, toHexString(k, value), basis);
}

int generateNpn4(const std::string &basis, std::string &filename) {
  eda::gate::optimizer::NpnDatabase db;
  constexpr uint8_t k = 4;

  for (size_t i = 0; i < eda::gate::optimizer::npn4Num; ++i) {
    const uint64_t mincode = eda::gate::optimizer::npn4[i];

    auto subnetObject = synthesize(k, mincode, basis);
    if (subnetObject.isNull()) {
      return 1;
    }
    db.push(subnetObject.make());
  }

  if (filename.empty()) {
    filename = eda::env::getHomePath() / "output" / "db";
    std::filesystem::create_directory(eda::env::getHomePath() / "output");
  }
  db.exportTo(filename);
  return 0;
}

int main(int argc, char **argv) {
  std::string filename;
  if (argc > 2) {
    std::copy_n(argv[2], strlen(argv[2]), std::back_inserter(filename));
  }
  if (argc > 1) {
    const std::string basis(argv[1], strlen(argv[1]));
    return generateNpn4(basis, filename);
  }
  std::cout << "Print basis!" << std::endl << std::endl;
  printUsage();
  return 1;
}
