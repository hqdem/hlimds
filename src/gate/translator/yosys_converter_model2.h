//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/printer/printer.h"
#include "gate/translator/model2.h"

#include <kernel/yosys.h>

/**
 * The class encapsulates Yosys-based Verilog frontend and provides
 * utilities to translate an input to Model 2. Instances of the class
 * initialize the Yosys framework on construction and finalize it on
 * destruction. Yosys has issues preventing to use it repeatedly
 * after shutdown.
 */

class YosysConverterModel2 {

  using CellID = eda::gate::model::CellID;
  using CellTypeID = eda::gate::model::CellTypeID;
  using CellSymbol = eda::gate::model::CellSymbol;
  template<typename T>
  using IdDict = Yosys::hashlib::dict<Yosys::RTLIL::IdString, T>;
  using LinkEnd = eda::gate::model::LinkEnd;
  using LinkList = eda::gate::model::Cell::LinkList;
  using NetBuilder = eda::gate::model::NetBuilder;
  using NetID = eda::gate::model::NetID;

  std::ostream &debug;

  static std::ostream devnull;

  enum Operator {
    O_Add,
    O_Sub,
    O_Mul,
    O_Div,
    O_Not,
    O_And,
    O_Nand,
    O_Or,
    O_Nor,
    O_Orr,
    O_Andr,
    O_Xorr,
    O_Xor,
    O_Xnor,
    O_Cat,
    O_Mux,
    O_Shr,
    O_Shl,
    O_Dshr,
    O_Dshl,
    O_Leq,
    O_Lt,
    O_Geq,
    O_Gt,
    O_Neg,
    O_Neq,
    O_Eq,
    O_Pad,
    O_Dff,
    O_Dffrs,
    O_Dffe,
    O_Sdff,
    O_Sdffe,
    O_Sdffce,
    O_Pmux,
    O_Init,
    O_Dlatch,
    O_Pos,
    O_Boolr,
    O_Xnorr,
    O_Eqx,
    O_Nex,
    O_Pow,
    O_Mod,
    O_Divfloor,
    O_Modfloor,
    O_Adffe,
    O_Aldffe,
    O_Dffsre,
    O_Aldff,
    O_Dffsr,
    O_Adlatch,
    O_Dlatchsr,
    O_Sr
  };

  enum Mode {
    M_Input,
    M_Output,
    M_Wire,
    M_Const
  };

  struct Signal {
    std::vector<int> chunks;
  };

  struct Attributes {
    std::vector<std::uint16_t> widthIn;
    std::vector<std::uint16_t> widthOut;
    Attributes() = default;
    Attributes(std::vector<std::uint16_t> wIn, std::vector<std::uint16_t> wOut) :
      widthIn(wIn), widthOut(wOut) {}
  };

  struct Operators {
    CellTypeID cellTypeId;
    Operator operator_;
    Operators() = default;
    Operators(CellTypeID cellTypeId, Operator op) :
      cellTypeId(cellTypeId), operator_(op) {}
  };

  std::vector<NetBuilder> stackNetBuilder;
  std::vector<NetID> listNetID;

  std::map<int, Yosys::RTLIL::Module *> modulesMap;
  std::map<std::string, Yosys::RTLIL::Module *> modulesNameMap;
  std::string nameTopModule;

  struct PortsInfo {
    std::map<std::string, int> ports;
    std::map<int, Mode> portsMode;
    std::vector<int> orderInputs;
    std::vector<int> orderOutputs;
  };

  struct Subnet {
    std::map<Signal *, Signal *> yosysCells;
    std::map<Signal *, Operators> operators;
    std::map<int, LinkEnd> entitiesLinks;
    std::map<Signal *, CellID> cellCompliance;
    std::map<int, Mode> entitiesMode;
    int indexNew = -1;
  };

  Subnet curModule;
  std::vector<Signal *> signals;

public:

  YosysConverterModel2(const YosysToModel2Config &cfg);

  NetID getNetID();

  ~YosysConverterModel2();

private:

  int getNewIndex();

  void deterTopModule(
    Yosys::RTLIL::Design &design, const std::string &topModule);

  static std::string operatorToString(Operator op);

  void readModules(const Yosys::RTLIL::Design &des);

  std::string readIdString(const Yosys::RTLIL::IdString &str);

  void addCell(CellID &cell);

  void walkWires(const IdDict<Yosys::RTLIL::Wire *> &ywires);

  void insertModeData(int idWire, Mode mode);

  std::vector<int> deterConst(const Yosys::RTLIL::SigChunk &sigWire);

  std::vector<int> deterSigSpecBit(const Yosys::RTLIL::SigChunk &chunk);

  static void appendVec(
      std::vector<int> &target, const std::vector<int> &source);

  std::vector<int> deterSigSpec(const Yosys::RTLIL::SigSpec &sigWire);

  static std::vector<int> combineVectors(
      const std::vector<std::vector<int>> &vectors);

  static std::vector<std::uint16_t> combineVectorsSize(
      const std::vector<std::vector<int>> &vectors);

  void makeRestCell(
      int indexOperator, std::vector<int> &q, std::vector<std::vector<int>> &leafs, bool sign = false);

  void makeDFF(const Yosys::RTLIL::Cell *cell);

  void makeDFFSR(const Yosys::RTLIL::Cell *cell);

  void makeSR(const Yosys::RTLIL::Cell *cell);

  void makeALDFF(const Yosys::RTLIL::Cell *cell);

  void makeALDFFE(const Yosys::RTLIL::Cell *cell);

  void makeDlatch(const Yosys::RTLIL::Cell *cell);

  void makeDlatchsr(const Yosys::RTLIL::Cell *cell);

  void makeADlatch(const Yosys::RTLIL::Cell *cell);

  std::vector<int> determineInitValue(const IdDict<Yosys::RTLIL::Const> &parms);

  void makeADFF(const Yosys::RTLIL::Cell *cell);

  void makeADFFE(const Yosys::RTLIL::Cell *cell);

  void makeDFFE(const Yosys::RTLIL::Cell *cell);

  void makeDFFSRE(const Yosys::RTLIL::Cell *cell);

  void makeSDFFE(const Yosys::RTLIL::Cell *cell);

  void makeSDFFCE(const Yosys::RTLIL::Cell *cell);

  void makeSDFF(const Yosys::RTLIL::Cell *cell);

  CellSymbol getCellSymbolOperator(Operator op, bool sign);

  CellTypeID getCellType(Operator op, Attributes *attr, bool sign);

  Signal *makeSignal(std::vector<int> &chunks);

  void fillOperatorData(
      int typeFunction, Signal *lhs, Attributes *attrs, bool sign);

  void insertYosysCells(Signal *lhs, Signal *rhs);

  void makeSoftOperatorCell(
      int typeFunction, std::vector<int> &root, std::vector<int> &leaf, std::vector<std::uint16_t> &widthIn, std::vector<std::uint16_t> &widthOut, bool sign);

  void makeConnectArnity1(
      const int typeFunction, const IdDict<Yosys::RTLIL::SigSpec> &cons);

  void makeConnectArnity2(const Yosys::RTLIL::Cell *cell);

  void printCell(const Yosys::RTLIL::Cell *cell);

  void makeMux(const Yosys::RTLIL::Cell *cell);

  static bool isMux(int index);

  static bool isDFF(int index);

  static bool isDFFSR(int index);

  static bool isDlatch(int index);

  static bool isADlatch(int index);

  static bool isDlatchsr(int index);

  bool isInitCell(int typeFunction);

  void insertOperatorData(Signal *sig, Operators *op);

  void gatherPortsInfo(
      PortsInfo &portsInfo, std::vector<int> &root, const Yosys::RTLIL::Cell *cell);

  void fillInitCellData(Yosys::RTLIL::Module *nested, PortsInfo *portsInfo);

  void walkNestedModule(Yosys::RTLIL::Module *nested);

  void makeInitCells(const Yosys::RTLIL::Cell *cell);

  static bool isADFF(int type);

  static bool isADFFE(int type);

  static bool isDFFE(int type);

  static bool isDFFSRE(int type);

  static bool isSDFFE(int type);

  static bool isSDFF(int type);

  static bool isALDFF(int type);

  static bool isALDFFE(int type);

  static bool isSR(int type);

  static bool isSDFFCE(int type);

  void makeTriggerOrConnectAnity2(const Yosys::RTLIL::Cell *cell);

  void makeConnectArnity3(const Yosys::RTLIL::Cell *cell);

  void printDataCell(
      const Yosys::RTLIL::IdString &str, const Yosys::RTLIL::Cell *cell);

  void walkCells(const IdDict<Yosys::RTLIL::Cell *> &ycells);

  static bool isUnOperator(Operator func);

  static bool isBinOperator(Operator func);

  static bool isTernOperator(Operator func);

  static int determineTypeOperator(Operator func);

  CellTypeID getOpTypeId(Signal *lhs);

  Operator getOperator(Signal *lhs);

  LinkList makeEmptyLinkList(size_t length);

  void insertEntityLinks(int entity, int port, CellID idCell);

  LinkEnd getEntityLinks(int entity);

  void insertCellCompliance(Signal *lhs, CellID idCell);

  CellID getCellCompliance(Signal *lhs);

  void buildEmptyCells();

  Mode getMode(int entity);

  void makeConnectionsCells();

  void walkYosysCells();

  void walkConnections(
      const std::vector<std::pair<Yosys::RTLIL::SigSpec, Yosys::RTLIL::SigSpec>> &cons);

  void insertPortMode(PortsInfo &pInfo, int port, Mode mode);

  void gatherModeInfo(PortsInfo &pInfo, Yosys::RTLIL::Module *nested);

  void printConnections(
      const Yosys::RTLIL::SigSpec &op1, const Yosys::RTLIL::SigSpec &op2);

  void walkParameteres(
      const Yosys::hashlib::idict<Yosys::RTLIL::IdString> &avail_parameters);

  void walkPorts(const std::vector<Yosys::RTLIL::IdString> &ports, bool buildOut);

  void walkMemories(const IdDict<Yosys::RTLIL::Memory *> &processes);

  void walkModule(const Yosys::RTLIL::Module *m);

  static Operator logicFunction(int type);

  const char *SID_A = "\\A";
  const char *SID_ARST = "\\ARST";
  const char *SID_ARST_VALUE = "\\ARST_VALUE";
  const char *SID_B = "\\B";
  const char *SID_CLK = "\\CLK";
  const char *SID_D = "\\D";
  const char *SID_EN = "\\EN";
  const char *SID_S = "\\S";
  const char *SID_Q = "\\Q";
  const char *SID_Y = "\\Y";
  const char *SID_CLR = "\\CLR";
  const char *SID_SET = "\\SET";
  const char *SID_ALOAD = "\\ALOAD";
  const char *SID_SRST = "\\SRST";
  const char *SID_A_SIGNED = "\\A_SIGNED";
};
