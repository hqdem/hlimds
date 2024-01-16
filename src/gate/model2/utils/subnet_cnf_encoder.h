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

  SubnetEncoderContext(Solver &solver, size_t expectedVars): solver(solver) {
    vars.reserve(expectedVars);
  }

  Variable var(size_t idx) const {
    return vars[idx];
  }

  Literal lit(size_t idx, bool sign) const {
    return eda::gate::solver::makeLit(var(idx), sign);
  }

  Literal lit(const Subnet::Link link, bool sign) const {
    return lit(link.idx, link.inv ^ sign);
  }

  Variable newVar() {
    return solver.newVar();
  }

  Variable newVar(size_t idx) {
    while (idx >= vars.size()) {
      vars.push_back(-1u);
    }

    return vars[idx] = newVar();
  }

  Literal newLit(bool sign) {
    return eda::gate::solver::makeLit(newVar(), sign);
  }

  Literal newLit(size_t idx, bool sign) {
    return eda::gate::solver::makeLit(newVar(idx), sign);
  }

private:
  Solver &solver;
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
    assert(subnet.getInNum() > 0);

    const auto &entries = subnet.getEntries();
    const auto expectedVars = static_cast<size_t>(1.25 * entries.size());
    SubnetEncoderContext context(solver, expectedVars);

    for (size_t i = 0; i < entries.size(); ++i) {
      const auto &cell = entries[i].cell;
      assert(!cell.isNull());

      if      (cell.isIn())   { encodeIn  (subnet, cell, i, context, solver); }
      else if (cell.isOut())  { encodeOut (subnet, cell, i, context, solver); }
      else if (cell.isZero()) { encodeZero(subnet, cell, i, context, solver); }
      else if (cell.isOne())  { encodeOne (subnet, cell, i, context, solver); }
      else if (cell.isBuf())  { encodeBuf (subnet, cell, i, context, solver); }
      else if (cell.isAnd())  { encodeAnd (subnet, cell, i, context, solver); }
      else if (cell.isOr())   { encodeOr  (subnet, cell, i, context, solver); }
      else if (cell.isXor())  { encodeXor (subnet, cell, i, context, solver); }
      else if (cell.isMaj())  { encodeMaj (subnet, cell, i, context, solver); }
      else                    { assert(false && "Unsupported operation"); }

      i += cell.more;
    }
  }

private:
  SubnetEncoder() {}

  void encodeIn(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 0);
    context.newVar(idx);
  }

  void encodeZero(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 0);
    solver.addClause(context.newLit(idx, 0));
  }

  void encodeOne(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 0);
    solver.addClause(context.newLit(idx, 1));
  }

  void encodeBuf(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 1);
    solver.encodeBuf(context.newLit(idx, 1), context.lit(cell.link[0], 1));
  }

  void encodeOut(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    encodeBuf(subnet, cell, idx, context, solver);
  }

  void encodeAnd(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity > 1);

    context.newVar(idx);

    Clause clause;
    clause.push(context.lit(idx, 1));

    for (size_t j = 0; j < cell.arity; ++j) {
      auto link = subnet.getLink(idx, j);
      clause.push(context.lit(link, 0));
      solver.addClause(context.lit(idx, 0), context.lit(link, 1));
    }

    solver.addClause(clause);
  }

  void encodeOr(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity > 1);

    context.newVar(idx);  

    Clause clause;
    clause.push(context.lit(idx, 0));

    for (size_t j = 0; j < cell.arity; ++j) {
      auto link = subnet.getLink(idx, j);
      clause.push(context.lit(link, 1));
      solver.addClause(context.lit(idx, 1), context.lit(link, 0));
    }

    solver.addClause(clause);
  }

  void encodeXor(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity > 1);

    size_t k = cell.arity;
    auto rhs = context.newLit(idx, 1);

    for (size_t j = 0; j < k - 1; ++j) {
      auto link1 = subnet.getLink(idx, j);
      auto link2 = subnet.getLink(idx, j + 1);

      auto lhs1 = context.lit(link1, 1);
      auto lhs2 = (j == k - 2) ? context.lit(link2, 1) : context.newLit(1);

      solver.encodeXor(rhs, lhs1, lhs2);
      rhs = lhs2;
    }
  }

  void encodeMaj(const Subnet &subnet, const Subnet::Cell &cell, size_t idx,
      SubnetEncoderContext &context, Solver &solver) const {
    assert(cell.arity == 3);

    auto lhs1 = context.lit(cell.link[0], 1);
    auto lhs2 = context.lit(cell.link[1], 1);
    auto lhs3 = context.lit(cell.link[2], 1);

    solver.encodeMaj(context.newLit(idx, 1), lhs1, lhs2, lhs3);
  }
};

} // namespace eda::gate::model
