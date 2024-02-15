//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "gate/solver/solver.h"
#include "util/singleton.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet Encoder Context
//===----------------------------------------------------------------------===//

class SubnetEncoderContext final {
public:
  using Variable = eda::gate::solver::Variable;
  using Literal  = eda::gate::solver::Literal;
  using Solver   = eda::gate::solver::Solver;

  static size_t estimateVarNum(const Subnet &subnet) {
    return static_cast<size_t>(1.25 * subnet.size());
  }

  SubnetEncoderContext(const Subnet &subnet, Solver &solver):
      solver(solver), next(subnet.size()) {
    vars.reserve(estimateVarNum(subnet));
  }

  Variable var(size_t idx, uint16_t out) const {
    return vars[pos(idx, out)];
  }

  Literal lit(size_t idx, uint16_t out, bool sign) const {
    return eda::gate::solver::makeLit(var(idx, out), sign);
  }

  Literal lit(const Subnet::Link link, bool sign) const {
    return lit(link.idx, link.out, (link.inv ^ sign));
  }

  Variable newVar() {
    return solver.newVar();
  }

  Literal newLit(bool sign = 1) {
    return eda::gate::solver::makeLit(newVar(), sign);
  }

  void setVars(size_t idx, uint16_t nOut) {
    assert(idx < next.size());
    next[idx] = pos(idx, nOut);

    while (next[idx] > vars.size()) {
      vars.push_back(-1u);
    }

    for (size_t p = pos(idx, 0); p < next[idx]; ++p) {
      vars[p] = newVar();
    }
  }

  void setVar(size_t idx) {
    return setVars(idx, 1);
  }

private:
  size_t pos(size_t idx, uint16_t out) const {
    assert(idx < next.size());
    return idx == 0 ? out : next[idx - 1] + out;
  }

  Solver &solver;
  std::vector<size_t> next;
  std::vector<Variable> vars;
};

//===----------------------------------------------------------------------===//
// Subnet Encoder
//===----------------------------------------------------------------------===//

class SubnetEncoder final : public util::Singleton<SubnetEncoder> {
  friend class util::Singleton<SubnetEncoder>;

public:
  using Variable = eda::gate::solver::Variable;
  using Literal  = eda::gate::solver::Literal;
  using Clause   = eda::gate::solver::Clause;
  using Solver   = eda::gate::solver::Solver;

  void encode(const Subnet &subnet, Solver &solver) const {
    SubnetEncoderContext context(subnet, solver);
    encode(subnet, context, solver);
  }

  void encode(const Subnet &subnet,
      SubnetEncoderContext &context, Solver &solver) const {

    const auto &entries = subnet.getEntries();
    for (size_t i = 0; i < entries.size(); ++i) {
      const auto &cell = entries[i].cell;
      assert(!cell.isNull());

           if (cell.isIn())   { encodeIn  (subnet, cell, i, context, solver); }
      else if (cell.isOut())  { encodeOut (subnet, cell, i, context, solver); }
      else if (cell.isZero()) { encodeZero(subnet, cell, i, context, solver); }
      else if (cell.isOne())  { encodeOne (subnet, cell, i, context, solver); }
      else if (cell.isBuf())  { encodeBuf (subnet, cell, i, context, solver); }
      else if (cell.isAnd())  { encodeAnd (subnet, cell, i, context, solver); }
      else if (cell.isOr())   { encodeOr  (subnet, cell, i, context, solver); }
      else if (cell.isXor())  { encodeXor (subnet, cell, i, context, solver); }
      else if (cell.isMaj())  { encodeMaj (subnet, cell, i, context, solver); }
      else {
        const auto &type = cell.getType();
        assert(type.isSubnet() && "Unsupported cell type");

        const auto &innerSubnet = type.getSubnet();
        assert(innerSubnet.getInNum() == type.getInNum());
        assert(innerSubnet.getOutNum() == type.getOutNum());

        // New subnet encoding context w/ the same solver.
        SubnetEncoderContext innerContext(innerSubnet, solver /* the same */);
        encode(innerSubnet, innerContext, solver);

        // Create boolean variables for the cell outputs.
        context.setVars(i, type.getOutNum());

        // Encode the input bindings.
        for (size_t j = 0; j < type.getInNum(); ++j) {
          auto link = subnet.getLink(i, j);
          // The j-th subnet input <= the j-th cell input.
          solver.encodeBuf(innerContext.lit(j, 0, 1), context.lit(link, 1));
        }

        // Encode the output bindings.
        const size_t out = innerSubnet.size() - innerSubnet.getOutNum();
        for (size_t j = 0; j < type.getOutNum(); ++j) {
          // The j-th cell output <= the j-th subnet output.
          const auto k = out + j;
          solver.encodeBuf(context.lit(i, j, 1), innerContext.lit(k, 0, 1));
        }
      }

      i += cell.more;
    }
  }

  void encodeEqual(const Subnet &subnet, SubnetEncoderContext &context,
      Solver &solver, Subnet::Link lhs, bool rhs) const {
    solver.addClause(context.lit(lhs, rhs));
  }

  void encodeEqual(const Subnet &subnet, SubnetEncoderContext &context,
      Solver &solver, Subnet::Link lhs, Subnet::Link rhs) const {
    solver.encodeBuf(context.lit(lhs, 1), context.lit(rhs, 1));
  }

private:
  SubnetEncoder() {}

  void encodeIn(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 0);
    context.setVar(idx);
  }

  void encodeZero(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 0);
    context.setVar(idx);
    solver.addClause(context.lit(idx, 0, 0));
  }

  void encodeOne(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 0);
    context.setVar(idx);
    solver.addClause(context.lit(idx, 0, 1));
  }

  void encodeBuf(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 1);
    context.setVar(idx);
    solver.encodeBuf(context.lit(idx, 0, 1), context.lit(cell.link[0], 1));
  }

  void encodeOut(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    encodeBuf(subnet, cell, idx, context, solver);
  }

  void encodeAnd(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity > 1);
    context.setVar(idx);

    Clause clause;
    clause.push(context.lit(idx, 0, 1));

    for (size_t j = 0; j < cell.arity; ++j) {
      const auto link = subnet.getLink(idx, j);
      clause.push(context.lit(link, 0));
      solver.addClause(context.lit(idx, 0, 0), context.lit(link, 1));
    }

    solver.addClause(clause);
  }

  void encodeOr(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity > 1);
    context.setVar(idx);  

    Clause clause;
    clause.push(context.lit(idx, 0, 0));

    for (size_t j = 0; j < cell.arity; ++j) {
      const auto link = subnet.getLink(idx, j);
      clause.push(context.lit(link, 1));
      solver.addClause(context.lit(idx, 0, 1), context.lit(link, 0));
    }

    solver.addClause(clause);
  }

  void encodeXor(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity > 1);
    context.setVar(idx);

    const size_t k = cell.arity;
    auto rhs = context.lit(idx, 0, 1);

    for (size_t j = 0; j < k - 1; ++j) {
      const auto link1 = subnet.getLink(idx, j);
      const auto link2 = subnet.getLink(idx, j + 1);

      const auto lhs1 = context.lit(link1, 1);
      const auto lhs2 = (j == k - 2) ? context.lit(link2, 1) : context.newLit();

      solver.encodeXor(rhs, lhs1, lhs2);
      rhs = lhs2;
    }
  }

  void encodeMaj(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 3);
    context.setVar(idx);

    const auto lhs1 = context.lit(cell.link[0], 1);
    const auto lhs2 = context.lit(cell.link[1], 1);
    const auto lhs3 = context.lit(cell.link[2], 1);

    solver.encodeMaj(context.lit(idx, 0, 1), lhs1, lhs2, lhs3);
  }
};

} // namespace eda::gate::model
