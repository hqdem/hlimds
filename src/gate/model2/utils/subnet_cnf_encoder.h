//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include "minisat/core/Solver.h"

#include <cassert>
#include <cstdint>
#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Context
//===----------------------------------------------------------------------===//

// MiniSAT-related types.
using Variable = Minisat::Var;
using Literal  = Minisat::Lit;
using Clause   = Minisat::vec<Literal>;
using Formula  = Minisat::Solver;

class Context final {
public:
  explicit Context(size_t nVars) {
    vars.reserve(nVars);
  }

  Variable var(size_t idx) const {
    return vars[idx];
  }

  Literal lit(size_t idx, bool sign) const {
    return Minisat::mkLit(var(idx), sign);
  }

  Literal lit(const Subnet::Link link, bool sign) const {
    return lit(link.idx, link.inv ^ sign);
  }

  Variable newVar() {
    return formula.newVar();
  }

  Variable newVar(size_t idx) {
    while (idx >= vars.size()) {
      vars.push_back(-1u);
    }

    return vars[idx] = newVar();
  }

  Literal newLit(bool sign) {
    return Minisat::mkLit(newVar(), sign);
  }

  Literal newLit(size_t idx, bool sign) {
    return Minisat::mkLit(newVar(idx), sign);
  }

  void encode(Literal lit) {
    formula.addClause(lit);
  }

  void encode(Literal lit1, Literal lit2) {
    formula.addClause(lit1, lit2);
  }

  void encode(Literal lit1, Literal lit2, Literal lit3) {
    formula.addClause(lit1, lit2, lit3);
  }

  void encode(const Clause &clause) {
    formula.addClause(clause);
  }

  void encodeBuf(Literal rhs, Literal lhs) {
    encode(~rhs,  lhs);
    encode( rhs, ~lhs);
  }

  void encodeAnd(Literal rhs, Literal lhs1, Literal lhs2) {
    encode( rhs, ~lhs1, ~lhs2);
    encode(~rhs,  lhs1);
    encode(~rhs,  lhs2);
  }

  void encodeOr(Literal rhs, Literal lhs1, Literal lhs2) {
    encode(~rhs,  lhs1,  lhs2);
    encode( rhs, ~lhs1);
    encode( rhs, ~lhs2);
  }

  void encodeXor(Literal rhs, Literal lhs1, Literal lhs2) {
    encode(~rhs, ~lhs1, ~lhs2);
    encode(~rhs,  lhs1,  lhs2);
    encode( rhs, ~lhs1,  lhs2);
    encode( rhs,  lhs1, ~lhs2);
  }

  void encodeMaj(Literal rhs, Literal lhs1, Literal lhs2, Literal lhs3) {
    auto tmp1 = newLit(1);
    auto tmp2 = newLit(1);
    auto tmp3 = newLit(1);

    // t1 = (x1 & x2), t2 = (x1 & x3), t3 = (x2 & x3).
    encodeAnd(tmp1, lhs1, lhs2);
    encodeAnd(tmp2, lhs1, lhs3);
    encodeAnd(tmp3, lhs2, lhs3);

    // y = maj(x1, x2, x3) = (t1 | t2 | t3).
    Clause clause;
    clause.push(~rhs);
    clause.push(tmp1);
    clause.push(tmp2);
    clause.push(tmp3);
    encode(clause);

    encode(rhs, ~tmp1);
    encode(rhs, ~tmp2);
    encode(rhs, ~tmp3);
  }

private:
  Formula formula;
  std::vector<uint32_t> vars;
};

//===----------------------------------------------------------------------===//
// Encoder
//===----------------------------------------------------------------------===//

inline void encodeIn(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t idx,
    Context &ctx) {
  assert(cell.arity == 0);
  ctx.newVar(idx);
}

inline void encodeZero(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t idx,
    Context &ctx) {
  assert(cell.arity == 0);
  ctx.encode(ctx.newLit(idx, 0));
}

inline void encodeOne(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t idx,
    Context &ctx) {
  assert(cell.arity == 0);
  ctx.encode(ctx.newLit(idx, 1));
}

inline void encodeBuf(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t idx,
    Context &ctx) {
  assert(cell.arity == 1);
  ctx.encodeBuf(ctx.newLit(idx, 1), ctx.lit(cell.link[0], 1));
}

inline void encodeOut(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t idx,
    Context &ctx) {
  encodeBuf(subnet, cell, idx, ctx);
}

inline void encodeAnd(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t idx,
    Context &ctx) {
  assert(cell.arity > 1);

  ctx.newVar(idx);

  Clause clause;
  clause.push(ctx.lit(idx, 1));

  for (size_t j = 0; j < cell.arity; ++j) {
    auto link = subnet.getLink(idx, j);
    clause.push(ctx.lit(link, 0));
    ctx.encode(ctx.lit(idx, 0), ctx.lit(link, 1));
  }

  ctx.encode(clause);
}

inline void encodeOr(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t idx,
    Context &ctx) {
  assert(cell.arity > 1);

  ctx.newVar(idx);  

  Clause clause;
  clause.push(ctx.lit(idx, 0));

  for (size_t j = 0; j < cell.arity; ++j) {
    auto link = subnet.getLink(idx, j);
    clause.push(ctx.lit(link, 1));
    ctx.encode(ctx.lit(idx, 1), ctx.lit(link, 0));
  }

  ctx.encode(clause);
}

inline void encodeXor(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t idx,
    Context &ctx) {
  assert(cell.arity > 1);

  size_t k = cell.arity;
  auto rhs = ctx.newLit(idx, 1);

  for (size_t j = 0; j < k - 1; ++j) {
    auto link1 = subnet.getLink(idx, j);
    auto link2 = subnet.getLink(idx, j + 1);

    auto lhs1 = ctx.lit(link1, 1);
    auto lhs2 = (j == k - 2) ? ctx.lit(link2, 1) : ctx.newLit(1);

    ctx.encodeXor(rhs, lhs1, lhs2);
    rhs = lhs2;
  }
}

inline void encodeMaj(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t idx,
    Context &ctx) {
  assert(cell.arity == 3);

  auto lhs1 = ctx.lit(cell.link[0], 1);
  auto lhs2 = ctx.lit(cell.link[1], 1);
  auto lhs3 = ctx.lit(cell.link[2], 1);

  ctx.encodeMaj(ctx.newLit(idx), lhs1, lhs2, lhs3);
}

inline void encode(const Subnet &subnet, Context &ctx) {
  assert(subnet.getInNum() > 0);

  const auto &entries = subnet.getEntries();

  for (size_t i = 0; i < entries.size(); ++i) {
    const auto &cell = entries[i].cell;
    assert(!cell.isNull());

    if      (cell.isIn())   { encodeIn  (subnet, cell, i, ctx); }
    else if (cell.isOut())  { encodeOut (subnet, cell, i, ctx); }
    else if (cell.isZero()) { encodeZero(subnet, cell, i, ctx); }
    else if (cell.isOne())  { encodeOne (subnet, cell, i, ctx); }
    else if (cell.isBuf())  { encodeBuf (subnet, cell, i, ctx); }
    else if (cell.isAnd())  { encodeAnd (subnet, cell, i, ctx); }
    else if (cell.isOr())   { encodeOr  (subnet, cell, i, ctx); }
    else if (cell.isXor())  { encodeXor (subnet, cell, i, ctx); }
    else if (cell.isMaj())  { encodeMaj (subnet, cell, i, ctx); }
    else                    { assert(false && "Unsupported operation"); }

    i += cell.more;
  }
}

} // namespace eda::gate::model
