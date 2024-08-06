//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/celltype.h"
#include "util/assert.h"

#include "circt/Dialect/FIRRTL/FIRRTLDialect.h"
#include "circt/Dialect/FIRRTL/FIRRTLOps.h"
#include "circt/Dialect/FIRRTL/FIRRTLTypes.h"

#include <map>
#include <vector>

using AddPrimOp = circt::firrtl::AddPrimOp;
using AndPrimOp = circt::firrtl::AndPrimOp;
using AndRPrimOp = circt::firrtl::AndRPrimOp;
using AsAsyncResetPrimOp = circt::firrtl::AsAsyncResetPrimOp;
using AsClockPrimOp = circt::firrtl::AsClockPrimOp;
using AsSIntPrimOp = circt::firrtl::AsSIntPrimOp; 
using AsUIntPrimOp = circt::firrtl::AsUIntPrimOp;
using AssertOp = circt::firrtl::AssertOp;
using AssumeOp = circt::firrtl::AssumeOp;
using BitsPrimOp = circt::firrtl::BitsPrimOp;
using CatPrimOp = circt::firrtl::CatPrimOp;
using CellSymbol = eda::gate::model::CellSymbol;
using ConstCastOp = circt::firrtl::ConstCastOp;
using ConstantOp = circt::firrtl::ConstantOp;
using CoverOp = circt::firrtl::CoverOp;
using DShlPrimOp = circt::firrtl::DShlPrimOp;
using DShlwPrimOp = circt::firrtl::DShlwPrimOp;
using DShrPrimOp = circt::firrtl::DShrPrimOp;
using Direction = circt::firrtl::Direction;
using DivPrimOp = circt::firrtl::DivPrimOp;
using EQPrimOp = circt::firrtl::EQPrimOp;
using FConnectLike = circt::firrtl::FConnectLike;
using FIntegerConstantOp = circt::firrtl::FIntegerConstantOp;
using FModuleOp = circt::firrtl::FModuleOp;
using FIRRTLBaseType = circt::firrtl::FIRRTLBaseType;
using GEQPrimOp = circt::firrtl::GEQPrimOp;
using GTPrimOp = circt::firrtl::GTPrimOp;
using HeadPrimOp = circt::firrtl::HeadPrimOp;
using InstanceOp = circt::firrtl::InstanceOp;
using IntegerType = circt::IntegerType;
using IntType = circt::firrtl::IntType;
using LEQPrimOp = circt::firrtl::LEQPrimOp;
using LTPrimOp = circt::firrtl::LTPrimOp;
using ModuleOp = mlir::ModuleOp;
using MulPrimOp = circt::firrtl::MulPrimOp;
using MultibitMuxOp = circt::firrtl::MultibitMuxOp;
using MuxPrimOp = circt::firrtl::MuxPrimOp;
using NEQPrimOp = circt::firrtl::NEQPrimOp;
using NegPrimOp = circt::firrtl::NegPrimOp;
using NotPrimOp = circt::firrtl::NotPrimOp;
using Operation = mlir::Operation;
using OrPrimOp = circt::firrtl::OrPrimOp;
using OrRPrimOp = circt::firrtl::OrRPrimOp;
using PadPrimOp = circt::firrtl::PadPrimOp;
using PrintFOp = circt::firrtl::PrintFOp;
using PropAssignOp = circt::firrtl::PropAssignOp;
using RegOp = circt::firrtl::RegOp;
using RegResetOp = circt::firrtl::RegResetOp;
using RemPrimOp = circt::firrtl::RemPrimOp;
using ShlPrimOp = circt::firrtl::ShlPrimOp;
using ShrPrimOp = circt::firrtl::ShrPrimOp;
using StopOp = circt::firrtl::StopOp;
using StrictConnectOp = circt::firrtl::StrictConnectOp;
using SubPrimOp = circt::firrtl::SubPrimOp;
using TailPrimOp = circt::firrtl::TailPrimOp;
using Type = mlir::Type;
using Value = mlir::Value;
using WireOp = circt::firrtl::WireOp;
using XorPrimOp = circt::firrtl::XorPrimOp;
using XorRPrimOp = circt::firrtl::XorRPrimOp;

namespace firrtl = circt::firrtl;
namespace hw = circt::hw;
namespace model = eda::gate::model;

inline bool isInstance(const Operation *op) {
  return circt::isa<InstanceOp>(op);
}

inline bool isRegister(const Operation *op) {
  return circt::isa<RegOp>(op);
}

inline bool isRegisterWithReset(const Operation *op) {
  return circt::isa<RegResetOp>(op);
}

inline bool isAnyRegister(const Operation *op) {
  return (isRegister(op) || isRegisterWithReset(op));
}

inline bool isEqual(const Operation *op) {
  return circt::isa<EQPrimOp>(op);
}

inline bool isNotEqual(const Operation *op) {
  return circt::isa<NEQPrimOp>(op);
}

inline bool isLessThan(const Operation *op) {
  return circt::isa<LTPrimOp>(op);
}

inline bool isLessThanOrEqual(const Operation *op) {
  return circt::isa<LEQPrimOp>(op);
}

inline bool isGreaterThan(const Operation *op) {
  return circt::isa<GTPrimOp>(op);
}

inline bool isGreaterThanOrEqual(const Operation *op) {
  return circt::isa<GEQPrimOp>(op);
}

inline bool isNegation(const Operation *op) {
  return circt::isa<NegPrimOp>(op);
}

inline bool isAddition(const Operation *op) {
  return circt::isa<AddPrimOp>(op);
}

inline bool isSubtraction(const Operation *op) {
  return circt::isa<SubPrimOp>(op);
}

inline bool isMultiplication(const Operation *op) {
  return circt::isa<MulPrimOp>(op);
}

inline bool isDivision(const Operation *op) {
  return circt::isa<DivPrimOp>(op);
}

inline bool isReminder(const Operation *op) {
  return circt::isa<RemPrimOp>(op);
}

inline bool isMux(const Operation *op) {
  return circt::isa<MuxPrimOp>(op);
}

inline bool isMultibitMux(const Operation *op) {
  return circt::isa<MultibitMuxOp>(op);
}

inline bool isDynamicShiftLeft(const Operation *op) {
  return circt::isa<DShlPrimOp>(op);
}

inline bool isDynamicShiftLeftPreserveWidth(const Operation *op) {
  return circt::isa<DShlwPrimOp>(op);
}

inline bool isDynamicShiftRight(const Operation *op) {
  return circt::isa<DShrPrimOp>(op);
}

inline bool isSynthesizable(const Operation *op) {
  return (isEqual(op) || isNotEqual(op) || isLessThan(op) ||
          isLessThanOrEqual(op) || isGreaterThan(op) ||
          isGreaterThanOrEqual(op) || isNegation(op) ||
          isAddition(op) || isSubtraction(op) || isMultiplication(op) ||
          isDivision(op) || isReminder(op) ||
          isMux(op) || isMultibitMux(op) ||
          isDynamicShiftLeft(op) || isDynamicShiftLeftPreserveWidth(op) ||
          isDynamicShiftRight(op));
}

inline bool isAnd(const Operation *op) {
  return circt::isa<AndPrimOp>(op);
}

inline bool isOr(const Operation *op) {
  return circt::isa<OrPrimOp>(op);
}

inline bool isXor(const Operation *op) {
  return circt::isa<XorPrimOp>(op);
}

inline bool isNot(const Operation *op) {
  return circt::isa<NotPrimOp>(op);
}

inline bool isBoolLogic(const Operation *op) {
  return (isAnd(op) || isOr(op) || isXor(op) || isNot(op));
}

inline bool isAndReduce(const Operation *op) {
  return circt::isa<AndRPrimOp>(op);
}

inline bool isOrReduce(const Operation *op) {
  return circt::isa<OrRPrimOp>(op);
}

inline bool isXorReduce(const Operation *op) {
  return circt::isa<XorRPrimOp>(op);
}

inline bool isBoolLogicReduce(const Operation *op) {
  return (isAndReduce(op) || isOrReduce(op) || isXorReduce(op));
}

inline bool isWire(const Operation *op) {
  return circt::isa<WireOp>(op);
}

inline bool isAssert(const Operation *op) {
  return circt::isa<AssertOp>(op);
}

inline bool isAssume(const Operation *op) {
  return circt::isa<AssumeOp>(op);
}

inline bool isCover(const Operation *op) {
  return circt::isa<CoverOp>(op);
}

inline bool isStop(const Operation *op) {
  return circt::isa<StopOp>(op);
}

inline bool isPrintFormattedString(const Operation *op) {
  return circt::isa<PrintFOp>(op);
}

inline bool isPropertyAssignment(const Operation *op) {
  return circt::isa<PropAssignOp>(op);
}

inline bool isConstant(const Operation *op) {
  return circt::isa<ConstantOp>(op);
}

inline bool isFIRRTLModule(const Operation *op) {
  return circt::isa<FModuleOp>(op);
}

inline bool isStrictConnect(const Operation *op) {
  return circt::isa<StrictConnectOp>(op);
}

inline bool isInteger(const Operation *op) {
  return circt::isa<FIntegerConstantOp>(op);
}

inline bool isOmitted(const Operation *op) {
  return (isAssert(op) || isAssume(op) || isCover(op) || isStop(op) ||
          isPrintFormattedString(op) || isPropertyAssignment(op) ||
          isConstant(op) || isWire(op) || isFIRRTLModule(op) ||
          isStrictConnect(op) || isInteger(op));
}

inline bool isPad(const Operation *op) {
  return circt::isa<PadPrimOp>(op);
}

inline bool isShiftLeft(const Operation *op) {
  return circt::isa<ShlPrimOp>(op);
}

inline bool isShiftRight(const Operation *op) {
  return circt::isa<ShrPrimOp>(op);
}

inline bool isBits(const Operation *op) {
  return circt::isa<BitsPrimOp>(op);
}

inline bool isConcatenation(const Operation *op) {
  return circt::isa<CatPrimOp>(op);
}

inline bool isHead(const Operation *op) {
  return circt::isa<HeadPrimOp>(op);
}

inline bool isTail(const Operation *op) {
  return circt::isa<TailPrimOp>(op);
}

inline bool isConstCast(const Operation *op) {
  return circt::isa<ConstCastOp>(op);
}

inline bool isAsClock(const Operation *op) {
  return circt::isa<AsClockPrimOp>(op);
}

inline bool isAsAsyncReset(const Operation *op) {
  return circt::isa<AsAsyncResetPrimOp>(op);
}

inline bool isAsUInt(const Operation *op) {
  return circt::isa<AsUIntPrimOp>(op);
}

inline bool isAsSInt(const Operation *op) {
  return circt::isa<AsSIntPrimOp>(op);
}

inline bool isBitManipulation(const Operation *op) {
  return (isPad(op) || isShiftLeft(op) || isShiftRight(op) || isBits(op) ||
          isConcatenation(op) || isHead(op) || isTail(op) || isConstCast(op) ||
          isAsClock(op) || isAsAsyncReset(op) || isAsSInt(op) || isAsUInt(op));
}

inline bool isSimpleLinkMove(const Operation *op) {
  return (isConstCast(op) || isAsClock(op) ||
          isAsAsyncReset(op) || isAsSInt(op) || isAsUInt(op));
}

inline bool isSigned(Operation *operation) {
  auto type = operation->getOperand(0).getType();
  return circt::firrtl::type_isa<IntType>(type);
}

inline uint getTypeWidth(const Type type) {
  auto &&firType = hw::type_cast<FIRRTLBaseType>(type);
  int typeWidthSentinel = firType.getBitWidthOrSentinel();

  uassert((typeWidthSentinel > 0), "Type width cannot be deduced!");

  return static_cast<uint>(typeWidthSentinel);
}

uint findOpOperandNumber(const Value val,
                         Operation *op,
                         FModuleOp fModuleOp);
uint findOpResultNumber(const Value val,
                        Operation *op,
                        FModuleOp fModuleOp);

uint getInCount(Operation *op);
uint getBitWidthIn(Operation *op);
uint getOutCount(Operation *op);
uint getBitWidthOut(Operation *op);

Operation *getSourceOperation(Operation *destOp, const Value operand);

Value getDestValue(Operation *destOp, const uint inNum);

CellSymbol getCellSymbol(Operation *op);

uint getNetInPortNum(Operation *op,
                     const uint portNum,
                     const uint bitNum);