//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"
#include "gate/optimizer/npn.h"

#include <percy/percy.hpp>

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

inline std::string toHexString(uint8_t k, uint64_t value) {
  assert(k <= 6);
  assert(value <= (1ull << (1 << k)) - 1);

  std::stringstream ss;
  ss << std::setfill('0') << std::setw(k) << std::hex << value;
  return ss.str();
}

inline eda::gate::model::SubnetID makeSubnet(uint8_t k, const percy::chain &chain) {
  const size_t nIn = chain.get_nr_inputs();
  const size_t nStep = chain.get_nr_steps();
  const size_t nOut = chain.get_nr_outputs();

  assert(k >= nIn);

  eda::gate::model::SubnetBuilder builder;
  std::vector<eda::gate::model::Subnet::Link> links(nIn + nStep);

  size_t j = 0;

  for (size_t i = 0; i < k; ++i) {
    const auto link = builder.addInput();
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
      links[j++] = builder.addCell(
          eda::gate::model::AND, links[args[0]], ~links[args[1]]);
      break;
    case 0x4:
      links[j++] = builder.addCell(
          eda::gate::model::AND, ~links[args[0]], links[args[1]]);
      break;
    case 0x6:
      links[j++] = builder.addCell(
          eda::gate::model::XOR, links[args[0]], links[args[1]]);
      break;
    case 0x8:
      links[j++] = builder.addCell(
          eda::gate::model::AND, links[args[0]], links[args[1]]);
      break;
    case 0xe:
      links[j++] = ~builder.addCell(
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
    
    const auto link = (var == 0) ? builder.addCell(eda::gate::model::ZERO) : links[var - 1];
    builder.addOutput(inv ? ~link : link);
  }

  return builder.make();
}

inline eda::gate::model::SubnetID synthesize(uint8_t k, uint64_t value) {
  kitty::dynamic_truth_table func(k);
  kitty::create_from_hex_string(func, toHexString(k, value));

  percy::spec spec;
  spec.set_nr_out(1);
  spec[0] = func;

  percy::chain chain;
  const auto result = synthesize(spec, chain);
  assert(result == percy::success);

  return makeSubnet(k, chain);
}

void generateXagNpn4() {
  constexpr uint8_t k = 4;

  for (size_t i = 0; i < eda::gate::optimizer::npn4Num; ++i) {
    const uint64_t mincode = eda::gate::optimizer::npn4[i];

    std::cout << toHexString(k, mincode) << std::endl;
    auto subnetID = synthesize(k, mincode);

    std::cout << eda::gate::model::Subnet::get(subnetID) << std::endl;
  }
}

int main() {
  generateXagNpn4();
}
