//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "firrtl_net_utils.h"

#include <iostream>

uint findOpOperandNumber(const Value val,
                         Operation *op,
                         FModuleOp fModuleOp) {
  uint opOperandNum = 0;
  // If the source operation does not exist - it is the output.
  if (op == nullptr) {
    uint outCount = 0;
    for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
      if (fModuleOp.getPortDirection(i) == Direction::Out) {
        auto &&outputValue = fModuleOp.getArgument(i);
        if (outputValue == val) {
          opOperandNum = outCount;
          return opOperandNum;
        }
        outCount++;
      }
    }
  } else {
    // 'InstanceOp's are processed differently from other operations.
    if (isInstance(op)) {
      auto instanceOp = mlir::dyn_cast<InstanceOp>(op);
      uint argumentCount = 0;
      for (uint i = 0; i < instanceOp->getNumResults(); i++) {
        if (instanceOp.getPortDirection(i) == Direction::In) {
          if (instanceOp->getResult(i) == val) {
            opOperandNum = argumentCount;
            return opOperandNum;
          }
          argumentCount++;
        }
      }
    } else if (isAnyRegister(op)) {
      uint i = 0;
      if (op->getResult(i) == val) {
        opOperandNum = i;
        return opOperandNum;
      }
      i++;
      for (; i < op->getNumOperands() + 1; i++) {
        if (op->getOperand(i - 1) == val) {
          opOperandNum = i;
          return opOperandNum;
        }
      }
    } else if (isWire(op)) {
      if (op->getResult(0) == val) {
        opOperandNum = 0;
        return opOperandNum;
      }
    } else {
      for (uint i = 0; i < op->getNumOperands(); i++) {
        if (op->getOperand(i) == val) {
          opOperandNum = i;
          return opOperandNum;
        }
      }
    }
  }
  return opOperandNum;
}

uint findOpResultNumber(const Value val,
                        Operation *op,
                        FModuleOp fModuleOp) {
  uint opResultNum = 0;
  // If the source operation does not exist - it is the input.
  if (op == nullptr) {
    uint inCount = 0;
    for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
      if (fModuleOp.getPortDirection(i) == Direction::In) {
        auto &&inputValue = fModuleOp.getArgument(i);
        if (inputValue == val) {
          opResultNum = inCount;
          return opResultNum;
        }
        inCount++;
      }
    }
    for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
       if (fModuleOp.getPortDirection(i) == Direction::Out) {
        auto &&inputValue = fModuleOp.getArgument(i);
        if (inputValue == val) {
          opResultNum = inCount;
          return opResultNum;
        }
        inCount++;
      }
    }
  } else {
    // 'InstanceOp's are processed differently from other operations.
    if (isInstance(op)) {
      auto instanceOp = mlir::dyn_cast<InstanceOp>(op);
      uint resultCount = 0;
      for (uint i = 0; i < instanceOp->getNumResults(); i++) {
        if (instanceOp.getPortDirection(i) == Direction::Out) {
          if (instanceOp->getResult(i) == val) {
            opResultNum = resultCount;
            return opResultNum;
          }
          resultCount++;
        }
      }
    } else {
      for (uint i = 0; i < op->getNumResults(); i++) {
        if (op->getResult(i) == val) {
          opResultNum = i;
          return opResultNum;
        }
      }
    }
  }
  return opResultNum;
}

uint getInCount(Operation *op) {
  uint inCount = 0;
  if (isInstance(op)) {
    auto instanceOp = mlir::dyn_cast<InstanceOp>(op);
    for (uint i = 0; i < instanceOp->getNumResults(); i++) {
      if (instanceOp.getPortDirection(i) == Direction::In) {
        inCount++;
      }
    }
  } else if (isRegister(op)) {
    inCount = 2;
  } else if (isRegisterWithReset(op)) {
    inCount = 4;
  } else if (isWire(op)) {
    inCount = 1;
  } else {
    inCount = op->getNumOperands();
  }
  return inCount;
}

std::vector<uint16_t> getPortWidthIn(Operation *op) {
  std::vector<uint16_t> bitWidthIn;
  if (isInstance(op)) {
    auto instanceOp = mlir::dyn_cast<InstanceOp>(op);
    for (size_t i = 0; i < instanceOp->getNumResults(); i++) {
      if (instanceOp.getPortDirection(i) == Direction::In) {
        uint16_t inputWidth = getResultWidth(instanceOp, i);
        bitWidthIn.push_back(inputWidth);
      }
    }
  } else if (isRegister(op) || isWire(op)) {
    bitWidthIn.push_back(getResultWidth(op, 0));
    // Register must have a clock input.
    if (isRegister(op)) {
      bitWidthIn.push_back(1);
    }
  } else if (isRegisterWithReset(op)) {
      // RegReset has a reset value of arbitrary width.
      for (size_t i = 0; i < op->getNumOperands(); i++) {
        uint16_t inputWidth = getOperandWidth(op, i);
        bitWidthIn.push_back(inputWidth);
      }
      for (size_t i = 0; i < op->getNumResults(); i++) {
        uint16_t inputWidth = getResultWidth(op, i);
        bitWidthIn.push_back(inputWidth);
      }
  } else {
    for (auto &&operandType : op->getOperandTypes()) {
      uint16_t inputWidth = getTypeWidth(operandType);
      bitWidthIn.push_back(inputWidth);
    }
  }
  return bitWidthIn;
}

uint getOutCount(Operation *op) {
  uint outCount = 0;
  if (isInstance(op)) {
    auto instanceOp = mlir::dyn_cast<InstanceOp>(op);
    for (size_t i = 0; i < instanceOp->getNumResults(); i++) {
      if (instanceOp.getPortDirection(i) == Direction::Out) {
        outCount++;
      }
    }
  } else if (isAnyRegister(op) || isWire(op)) {
    outCount = 1;
  } else {
    outCount = op->getNumResults();
  }
  return outCount;
}

std::vector<uint16_t> getPortWidthOut(Operation *op) {
  std::vector<uint16_t> bitWidthOut;
  if (isInstance(op)) {
    auto instanceOp = mlir::dyn_cast<InstanceOp>(op);
    for (size_t i = 0; i < instanceOp->getNumResults(); i++) {
      if (instanceOp.getPortDirection(i) == Direction::Out) {
        uint16_t outputWidth = getResultWidth(instanceOp, i);
        bitWidthOut.push_back(outputWidth);
      }
    }
  } else if (isAnyRegister(op)) {
    for (size_t i = 0; i < op->getNumResults(); i++) {
      uint16_t outputWidth = getResultWidth(op, i);
      bitWidthOut.push_back(outputWidth);
    }
  } else {
    for (auto &&resultType : op->getResultTypes()) {
      uint16_t outputWidth = getTypeWidth(resultType);
      bitWidthOut.push_back(outputWidth);
    }
  }
  return bitWidthOut;
}

Operation *getSourceOperation(Operation *destOp, const Value operand) {
  Operation *srcOp = nullptr;
  if (isInstance(destOp) || isWire(destOp)) {
    for (auto *user : operand.getUsers()) {
      if (auto connect = mlir::dyn_cast<StrictConnectOp>(user)) {
        if (connect.getDest() != operand) {
          continue;
        }
        srcOp = connect.getSrc().getDefiningOp();
      }
    }
  } else {
    srcOp = operand.getDefiningOp();
  }
  return srcOp;
}

Value getDestValue(Operation *destOp, const uint inNum) {
  Value operand;
  if (isInstance(destOp)) {
    operand = destOp->getResult(inNum);
  } else if (isWire(destOp)) {
    operand = destOp->getResult(0);
  } else {
    operand = destOp->getOperand(inNum);
  }
  return operand;
}

CellSymbol getCellSymbol(Operation *op) {
  CellSymbol cellSymbol;
  if (op) {
    if (isEqual(op)) {
      cellSymbol = isSigned(op) ? CellSymbol::EQs : CellSymbol::EQu;
    } else if (isNotEqual(op)) {
      cellSymbol = isSigned(op) ? CellSymbol::NEQs : CellSymbol::NEQu;
    } else if (isLessThan(op)) {
      cellSymbol = isSigned(op) ? CellSymbol::LTs : CellSymbol::LTu;
    } else if (isLessThanOrEqual(op)) {
      cellSymbol = isSigned(op) ? CellSymbol::LTEs : CellSymbol::LTEu;
    } else if (isGreaterThan(op)) {
      cellSymbol = isSigned(op) ? CellSymbol::GTs : CellSymbol::GTu;
    } else if (isGreaterThanOrEqual(op)) {
      cellSymbol = isSigned(op) ? CellSymbol::GTEs : CellSymbol::GTEu;
    } else if (isAddition(op)) {
      cellSymbol = CellSymbol::ADD;
    } else if (isSubtraction(op)) {
      cellSymbol = CellSymbol::SUB;
    } else if (isMultiplication(op)) {
      cellSymbol = isSigned(op) ? CellSymbol::MULs : CellSymbol::MULu;
    } else if (isDivision(op)) {
      cellSymbol = isSigned(op) ? CellSymbol::DIVs : CellSymbol::DIVu;
    } else if (isReminder(op)) {
      cellSymbol = isSigned(op) ? CellSymbol::REMs : CellSymbol::REMu;
    } else if (isMux(op)) {
      cellSymbol = CellSymbol::MUX2;
    } else if (isInstance(op)) {
      cellSymbol = CellSymbol::UNDEF;
    } else if (isAnd(op) || isAndReduce(op)) {
      cellSymbol = CellSymbol::AND;
    } else if (isOr(op) || isOrReduce(op)) {
      cellSymbol = CellSymbol::OR;
    } else if (isXor(op) || isXorReduce(op)) {
      cellSymbol = CellSymbol::XOR;
    } else if (isNot(op)) {
      cellSymbol = CellSymbol::NOT;
    } else if (isRegister(op)) {
      cellSymbol = CellSymbol::DFF;
    } else if (isRegisterWithReset(op)) {
      cellSymbol = CellSymbol::DFFrs;
    } else {
      cellSymbol = CellSymbol::UNDEF;
    }
  } else {
    cellSymbol = CellSymbol::OUT;
  }
  return cellSymbol;
}

std::vector<uint16_t> getModulePortWidths(FModuleOp fModuleOp, Direction dir) {
  std::vector<uint16_t> modulePortWidths;
  for (size_t i = 0; i < fModuleOp.getNumPorts(); i++) {
    if (fModuleOp.getPortDirection(i) == dir &&
        !(mlir::dyn_cast<PropertyType>(fModuleOp.getPortType(i)))) {
      uint16_t portWidth = getTypeWidth(fModuleOp.getPortType(i));
      modulePortWidths.push_back(portWidth);
    }
  }
  return modulePortWidths;
}

uint getNetInPortNum(Operation *op,
                     const uint portNum,
                     const uint bitNum) {
  uint netInPortNum = 0;
  if (!op) {
    return 0;
  }
  // 'InstanceOp's are processed differently from other operations.
  if (isInstance(op)) {
    auto instanceOp = mlir::dyn_cast<InstanceOp>(op);
    uint inputCount = 0;
    for (uint i = 0; i < op->getNumResults(); i++) {
      if (inputCount == portNum) {
        break;
      }
      if (instanceOp.getPortDirection(i) == Direction::In) {
        netInPortNum += getResultWidth(instanceOp, i);
        inputCount++;
      }
    }
    netInPortNum += bitNum;
  } else if (isBoolLogic(op) || isWire(op)) {
    netInPortNum = portNum;
  } else if (isAnyRegister(op)) {
    netInPortNum = portNum;
  } else {
    for (uint i = 0; i < portNum; i++) {
      netInPortNum += getOperandWidth(op, i);
    }
    netInPortNum += bitNum;
  }
  return netInPortNum;
}