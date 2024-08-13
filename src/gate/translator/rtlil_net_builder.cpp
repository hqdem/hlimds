#include "model2.h"

#include <gate/model/net.h>

#include <easylogging++.h>
#include <fmt/format.h>
#include <kernel/sigtools.h>
#include <kernel/yosys.h>

#include <algorithm>
#include <cstring>
#include <map>
#include <string_view>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define LOG_FMT(level, logger, fmtstr, ...) do { \
  CLOG(level, (logger)) << fmt::format((fmtstr), __VA_ARGS__); \
} while (0)

#define COPY_POLICY(Name, policy) \
  Name(const Name &) = policy; \
  Name &operator=(const Name &) = policy

#define MOVE_POLICY(Name, policy) \
  Name(Name &&) = policy; \
  Name &operator=(Name &&) = policy

#define COPY_MOVE_POLICY(Name, cpypol, movpol) \
  COPY_POLICY(Name, cpypol); \
  MOVE_POLICY(Name, movpol)

#define MOVE_ONLY(Name) COPY_MOVE_POLICY(Name, delete, default)
#define COPY_MOVE(Name) COPY_MOVE_POLICY(Name, default, default)

namespace RTLIL = Yosys::RTLIL;
namespace SID = Yosys::RTLIL::ID;
namespace model = eda::gate::model;

using model::LinkEnd;
using Yosys::SigMap;

template<>
struct fmt::formatter<RTLIL::IdString>: fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const RTLIL::IdString &s, FormatContext &ctx) {
    return formatter<std::string_view>::format(s.c_str(), ctx);
  }
};

static bool
isModuleInstance(const RTLIL::Cell &cell) {
  if (cell.module && cell.module->design) {
    return cell.module->design->module(cell.type);
  }
  return false;
}

static bool
isSeqCell(const RTLIL::Cell &cell) {
  static const RTLIL::IdString seqTypes[] = {
    ID($sr),
    ID($ff),
    ID($dff),
    ID($dffe),
    ID($dffsr),
    ID($dffsre),
    ID($adff),
    ID($adffe),
    ID($aldff),
    ID($aldffe),
    ID($sdff),
    ID($sdffe),
    ID($sdffce),
    ID($dlatch),
    ID($adlatch),
    ID($dlatchsr)
  };
  const auto *it = std::find(
      std::begin(seqTypes), std::end(seqTypes), cell.type);
  return it != std::end(seqTypes);
}

/** @brief Store a named port connection to `links` bitwise.
 *  Unconnected ports produce z-values according to `portWidth`.
 */
static void
getBitwisePortLinks(
    std::vector<RTLIL::SigBit> &links,
    const RTLIL::Cell &cell,
    const RTLIL::IdString &portName,
    int portWidth) {
  auto it = cell.connections().find(portName);
  if (it != cell.connections().end()) {
    const std::vector<RTLIL::SigBit> &bits = it->second.bits();
    links.insert(links.end(), bits.begin(), bits.end());
  } else {
    LOG_FMT(WARNING, "rtlil", "{}: Unconnected port {} in {} cell ({})",
        cell.get_src_attribute(), portName, cell.type, cell.name);
    links.insert(links.end(), portWidth, RTLIL::Sz);
  }
}

static void
getBitwiseLinks(
    std::vector<RTLIL::SigBit> &links,
    const RTLIL::Cell &cell,
    const std::vector<const RTLIL::Wire *> &ports) {
  for (const RTLIL::Wire *port: ports) {
    getBitwisePortLinks(links, cell, port->name, port->width);
  }
}

static uint16_t
getIntPar(const RTLIL::IdString &parName, const RTLIL::Cell &cell) {
  if (cell.hasParam(parName)) {
    return static_cast<uint16_t>(cell.getParam(parName).as_int());
  }
  return 0;
}

struct CellTypeInstance {
  model::CellSymbol kind;
  uint16_t widthY, widthA, widthB;

  CellTypeInstance(const RTLIL::Cell &cell);

  CellTypeInstance(
      model::CellSymbol kind,
      uint16_t widthY,
      uint16_t widthA,
      uint16_t widthB)
      : kind(kind), widthY(widthY), widthA(widthA), widthB(widthB)
  {}

  COPY_MOVE(CellTypeInstance);

  bool operator<(const CellTypeInstance &that) const {
    return memcmp(this, &that, sizeof(that)) < 0;
  }

  enum Kind {
    Mux = 0b00,
    Unary = 0b01,
    Binary = 0b11,
  };

  bool isValid() const {
    return kind != model::UNDEF;
  }

  bool isGate() const {
    return widthY == 1 && widthA <= 1 && widthB <= 1;
  }

  Kind getKind() const {
    return static_cast<Kind>(bool(widthA) | (bool(widthB) << 1));
  }

  uint16_t getNumInputPorts() const {
    uint16_t n = widthA + widthB;
    return (n != 0) ? n : widthY * 2 + 1;
  }

  void getInputPortWidths(std::vector<uint16_t> &widths) const {
    switch (getKind()) {
    case Unary:
      widths = { widthA };
      break;

    case Binary:
      widths = { widthA, widthB };
      break;

    case Mux:
      widths = { 1, widthY, widthY };
      break;
    }
  }

  std::string format(const RTLIL::IdString &name) const {
    std::string_view fmt;
    switch (getKind()) {
    case Unary:
      fmt = "\\{0}#{3}-{1}";
      break;

    case Binary:
      fmt = "\\{0}#{3}-{1}/{2}";
      break;

    case Mux:
      fmt = "\\{0}#{3}";
      break;
    }
    return fmt::format(fmt, name, widthA, widthB, widthY);
  }
};

struct ModuleType {
  model::CellTypeID typeId;
  uint16_t nInputBits, nOutputBits;
  std::vector<const RTLIL::Wire *> inputs, outputs;

  ModuleType(const RTLIL::Module &m);

  MOVE_ONLY(ModuleType);
};

static bool
orderWiresByPortId(const RTLIL::Wire *lhs, const RTLIL::Wire *rhs) {
  return lhs->port_id < rhs->port_id;
}

ModuleType::ModuleType(const RTLIL::Module &m) {
  uint16_t nIn = 0, nOut = 0;
  for (const auto &[name, wire]: m.wires_) {
    if (wire->port_id > 0) {
      if (wire->port_input) {
        inputs.push_back(wire);
        nIn += wire->width;
      } else {
        outputs.push_back(wire);
        nOut += wire->width;
      }
    }
  }
  std::sort(inputs.begin(), inputs.end(), orderWiresByPortId);
  std::sort(outputs.begin(), outputs.end(), orderWiresByPortId);

  typeId = model::makeSoftType(
      model::UNDEF, m.name.str(), model::OBJ_NULL_ID, nIn, nOut);
  nInputBits = nIn;
  nOutputBits = nOut;
}

struct BitProvider {
  BitProvider(model::NetBuilder &builder)
      : bit { model::OBJ_NULL_ID, model::OBJ_NULL_ID }
      , builder(builder)
  {}

  COPY_POLICY(BitProvider, delete);

  model::CellID getBit(bool value) {
    model::CellID cellId = bit[value];
    if (cellId == model::OBJ_NULL_ID) {
      cellId = model::makeCell((value) ? model::ONE : model::ZERO);
      builder.addCell(cellId);
      bit[value] = cellId;
    }
    return cellId;
  }

private:
  model::CellID bit[2];
  model::NetBuilder &builder;
};

struct DesignBuilder;
struct ModuleBuilder {
  template<typename T>
  struct PairFirstLess {
    bool operator()(const T &lhs, const T &rhs) const {
      return std::less()(lhs.first, rhs.first);
    }
  };

  struct DrivenPort {
    model::CellID cellId;
    RTLIL::SigBit driver;
    uint16_t portId;
  };

  using CellPair = std::pair<const RTLIL::Cell *, model::CellID>;
  using CellLess = PairFirstLess<CellPair>;

  using PortPair = std::pair<RTLIL::SigBit, model::LinkEnd>;
  using PortLess = PairFirstLess<PortPair>;

  ModuleBuilder(DesignBuilder &ctx)
      : ctx(ctx)
  {}

  DesignBuilder &ctx;

  std::vector<CellPair> cells;
  std::vector<PortPair> drivingPorts;
  std::vector<DrivenPort> drivenPorts;
  std::vector<RTLIL::SigBit> bitBuffer;
  SigMap sigmap;

  model::NetID translateModule(const RTLIL::Module &m) {
    model::NetBuilder builder;

    allocateInputPorts(builder, m);
    allocateDrivingCells(builder, m);
    connectDrivenPorts(builder);
    allocateOutputPorts(builder, m);
    reset();

    return builder.make();
  }

private:
  void allocateInputPorts(
      model::NetBuilder &builder, const RTLIL::Module &m);

  void allocateOutputPorts(
      model::NetBuilder &builder, const RTLIL::Module &m);

  void allocateDrivingCells(
      model::NetBuilder &builder, const RTLIL::Module &m);

  void allocateModuleInstance(
      model::NetBuilder &builder, const RTLIL::Cell &cell);

  void allocateSeqCell(
      model::NetBuilder &builder, const RTLIL::Cell &cell);

  void allocateCombCell(
      model::NetBuilder &builder, const RTLIL::Cell &cell);

  void synthesizeLogicOr(
      model::NetBuilder &builder, const RTLIL::Cell &cell);

  void synthesizeLogicAnd(
      model::NetBuilder &builder, const RTLIL::Cell &cell);

  model::CellID makeReduceOr(
      model::NetBuilder &builder, const std::vector<RTLIL::SigBit> &ports);

  model::CellID allocateCell(
      model::NetBuilder &builder,
      const RTLIL::Cell *cell,
      model::CellTypeID typeId,
      uint16_t nports) {
    std::vector<LinkEnd> inputs(nports, LinkEnd());
    model::CellID cellId = model::makeCell(typeId, inputs);

    builder.addCell(cellId);
    if (cell) {
      cells.emplace_back(cell, cellId);
    }
    return cellId;
  }

  void connectDrivingPorts(
      model::CellID cellId, const std::vector<RTLIL::SigBit> &bits) {
    uint16_t portId = 0;
    for (const RTLIL::SigBit &bit: bits) {
      if (bit.is_wire()) {
        drivingPorts.emplace_back(bit, LinkEnd(cellId, portId));
      }
      ++portId;
    }
  }

  void connectDrivenPorts(model::NetBuilder &builder);

  void connectSplitDrivenPorts(model::NetBuilder &builder);

  void allocateDrivenPorts(
      const std::vector<RTLIL::SigBit> ports, model::CellID cellId) {
    for (uint16_t i = 0; i < static_cast<uint16_t>(ports.size()); ++i) {
      DrivenPort port = { cellId, ports[i], i };
      drivenPorts.emplace_back(port);
    }
  }

  void connectCellPorts(
      model::NetBuilder &builder,
      model::CellID cellId,
      const std::vector<RTLIL::SigBit> &ports) {
    BitProvider provider(builder);

    uint16_t portId = 0;
    for (const RTLIL::SigBit &bit: ports) {
      LinkEnd link = getDrivingLink(bit, provider);
      builder.connect(cellId, portId, link);
      ++portId;
    }
  }

  LinkEnd getDrivingLink(const RTLIL::SigBit &bit, BitProvider &provider) {
    RTLIL::SigBit sig = sigmap(bit);
    if (sig.is_wire()) {
      const auto value = std::make_pair(sig, LinkEnd());
      auto it = std::lower_bound(
          drivingPorts.begin(), drivingPorts.end(), value, PortLess{});
      if (it->first == sig) {
        return it->second;
      }
      // undriven port
      return LinkEnd(provider.getBit(false));
    }
    return LinkEnd(provider.getBit(sig.data == RTLIL::S1));
  }

  void reset() {
    cells.clear();
    drivenPorts.clear();
    drivingPorts.clear();
    sigmap.clear();
    bitBuffer.clear();
  }
};

struct DesignBuilder {
  std::map<const RTLIL::Module *, ModuleType> moduleTypes;
  std::map<CellTypeInstance, model::CellTypeID> rtlTypes;

  void translateDesign(const RTLIL::Design &d) {
    for (const auto &[name, m]: d.modules_) {
      moduleTypes.emplace(m, ModuleType(*m));
    }

    ModuleBuilder builder(*this);
    for (const auto &[name, m]: d.modules_) {
      model::NetID netId = builder.translateModule(*m);
      model::CellTypeID typeId = getModuleType(*m).typeId;
      model::CellType::get(typeId).setNet(netId);
    }
  }

  const ModuleType &getModuleType(const RTLIL::Cell &cell) const {
    const RTLIL::Module *m = cell.module->design->module(cell.type);
    return moduleTypes.find(m)->second;
  }

  const ModuleType &getModuleType(const RTLIL::Module &m) const {
    return moduleTypes.find(&m)->second;
  }

  model::CellTypeID getInstanceCellTypeID(
      const CellTypeInstance cti, const RTLIL::IdString &typeName);
};

static bool
hasModule(std::string name, const RTLIL::Design &d) {
  if (name[0] != '\\') {
    name.insert(name.begin(), '\\');
  }
  return d.module(name);
}

model::CellTypeID
DesignBuilder::getInstanceCellTypeID(
    const CellTypeInstance inst,
    const RTLIL::IdString &name) {
  assert(inst.isValid());

  auto it = rtlTypes.find(inst);
  if (it != rtlTypes.end()) {
    return it->second;
  }
  if (inst.isGate()) {
    model::CellTypeID typeId = model::getCellTypeID(inst.kind);
    if (typeId != model::OBJ_NULL_ID) {
      rtlTypes.emplace(inst, typeId);
      return typeId;
    }
  }
  std::vector<uint16_t> inputs, outputs;
  inst.getInputPortWidths(inputs);
  outputs = { inst.widthY };
  model::CellTypeID typeId = model::makeSoftType(
      inst.kind, inst.format(name), model::OBJ_NULL_ID, inputs, outputs);
  rtlTypes.emplace(inst, typeId);

  return typeId;
}

static inline
RTLIL::SigBit
makeSigBit(const RTLIL::Wire *wire, int port) {
  return RTLIL::SigBit(const_cast<RTLIL::Wire *>(wire), port);
}

void
ModuleBuilder::allocateInputPorts(
    model::NetBuilder &builder, const RTLIL::Module &m) {
  for (const RTLIL::Wire *wire: ctx.getModuleType(m).inputs) {
    for (int i = 0; i < wire->width; ++i) {
      model::CellID cellId = model::makeCell(model::IN);
      builder.addCell(cellId);
      drivingPorts.emplace_back(makeSigBit(wire, i), LinkEnd(cellId));
    }
  }
}

void
ModuleBuilder::allocateOutputPorts(
    model::NetBuilder &builder, const RTLIL::Module &m) {
  BitProvider provider(builder);
  const ModuleType &type = ctx.getModuleType(m);
  for (const RTLIL::Wire *wire: type.outputs) {
    for (int i = 0; i < wire->width; ++i) {
      RTLIL::SigBit bit = makeSigBit(wire, i);
      model::CellID cellId =
          model::makeCell(model::OUT, getDrivingLink(bit, provider));
      builder.addCell(cellId);
    }
  }
}

model::CellID
ModuleBuilder::makeReduceOr(
    model::NetBuilder &builder, const std::vector<RTLIL::SigBit> &ports) {
  model::CellID cellId;

  const uint16_t width = static_cast<uint16_t>(ports.size());
  if (width > 1) {
    CellTypeInstance inst(model::ROR, 1, width, 0);
    model::CellTypeID typeId =
        ctx.getInstanceCellTypeID(inst, ID($reduce_or));
    cellId = allocateCell(builder, nullptr, typeId, width);
  } else {
    cellId = model::makeCell(model::BUF, LinkEnd());
    builder.addCell(cellId);
  }
  allocateDrivenPorts(ports, cellId);

  return cellId;
}

void
ModuleBuilder::synthesizeLogicOr(
    model::NetBuilder &builder, const RTLIL::Cell &cell) {
  const uint16_t width = static_cast<uint16_t>(
      getIntPar(SID::A_WIDTH, cell) + getIntPar(SID::B_WIDTH, cell));
  CellTypeInstance inst(model::ROR, 1, width, 0);
  model::CellTypeID typeId =
      ctx.getInstanceCellTypeID(inst, ID($reduce_or));
  model::CellID cellId = allocateCell(builder, &cell, typeId, width);
  connectDrivingPorts(cellId, cell.getPort(SID::Y).bits());
}

void
ModuleBuilder::synthesizeLogicAnd(
    model::NetBuilder &builder, const RTLIL::Cell &cell) {

  model::CellID cellA = makeReduceOr(builder, cell.getPort(SID::A).bits());
  model::CellID cellB = makeReduceOr(builder, cell.getPort(SID::B).bits());
  model::CellID cellAnd =
      model::makeCell(model::AND, LinkEnd(cellA), LinkEnd(cellB));
  builder.addCell(cellAnd);

  const std::vector<RTLIL::SigBit> &bits = cell.getPort(SID::Y);

  // Only LSB value matters
  drivingPorts.emplace_back(bits[0], LinkEnd(cellAnd));

  // Fill with 0s
  BitProvider provider(builder);
  for (int i = 1; i < getIntPar(SID::Y_WIDTH, cell); ++i) {
    drivingPorts.emplace_back(bits[i], LinkEnd(provider.getBit(0)));
  }
}

void
ModuleBuilder::allocateDrivingCells(
    model::NetBuilder &builder, const RTLIL::Module &m) {
  for (const auto &[name, cell]: m.cells_) {
    if (isModuleInstance(*cell)) {
      allocateModuleInstance(builder, *cell);
    } else if (isSeqCell(*cell)) {
      allocateSeqCell(builder, *cell);
    } else if (cell->type == ID($logic_or)) {
      synthesizeLogicOr(builder, *cell);
    } else if (cell->type == ID($logic_and)) {
      synthesizeLogicAnd(builder, *cell);
    } else {
      allocateCombCell(builder, *cell);
    }
  }
  std::sort(drivingPorts.begin(), drivingPorts.end(), PortLess{});
}

void
ModuleBuilder::allocateModuleInstance(
    model::NetBuilder &builder, const RTLIL::Cell &cell) {

  const ModuleType &type = ctx.getModuleType(cell);
  model::CellID cellId =
      allocateCell(builder, &cell, type.typeId, type.nInputBits);

  getBitwiseLinks(bitBuffer, cell, type.outputs);
  connectDrivingPorts(cellId, bitBuffer);
  bitBuffer.clear();
}

void
ModuleBuilder::connectSplitDrivenPorts(model::NetBuilder &builder) {
  const auto end = drivenPorts.end();
  auto it = drivenPorts.begin();
  while (it != end) {
    BitProvider provider(builder);
    model::CellID cellId = it->cellId;
    while (it != end && it->cellId == cellId) {
      LinkEnd link = getDrivingLink(it->driver,  provider);
      builder.connect(it->cellId, it->portId, link);
      ++it;
    }
  }
}

void
ModuleBuilder::connectDrivenPorts(model::NetBuilder &builder) {
  for (const auto &[cell, cellId]: cells) {
    if (isModuleInstance(*cell)) {
      const ModuleType &type = ctx.getModuleType(*cell);

      getBitwiseLinks(bitBuffer, *cell, type.inputs);
      connectCellPorts(builder, cellId, bitBuffer);
      bitBuffer.clear();
    } else {
      if (cell->input(SID::S)) {
        getBitwisePortLinks(bitBuffer, *cell, SID::S, 1);
      }
      if (cell->input(SID::A)) {
        getBitwisePortLinks(
            bitBuffer, *cell, SID::A, getIntPar(SID::A_WIDTH, *cell));
      }
      if (cell->input(SID::B)) {
        getBitwisePortLinks(
            bitBuffer, *cell, SID::B, getIntPar(SID::B_WIDTH, *cell));
      }
      connectCellPorts(builder, cellId, bitBuffer);
      bitBuffer.clear();
    }
  }
  connectSplitDrivenPorts(builder);
}

struct SeqParMapping {
  RTLIL::IdString name;
  uint16_t lo, hi;

  uint16_t get(bool flag) const {
    return (flag) ? hi : lo;
  }
};

static uint16_t
mapParameterValue(
    const RTLIL::IdString &parName, const RTLIL::Cell &cell, int portId) {
  static const SeqParMapping boolValues[] = {
    { SID::CLK_POLARITY, model::NEGEDGE, model::POSEDGE },
    { SID::SET_POLARITY, model::SETLVL0, model::SETLVL1 },
    { SID::EN_POLARITY, model::ENALVL0, model::ENALVL1 },
    { SID::CLR_POLARITY, model::RSTLVL0, model::RSTLVL1 },
    { SID::SRST_POLARITY, model::RSTLVL0, model::RSTLVL1 },
    { SID::ARST_POLARITY, model::RSTLVL0, model::RSTLVL1 }
  };
  static const SeqParMapping multibitValues[] = {
    { SID::SRST_VALUE, model::RSTVAL0, model::RSTVAL1 },
    { SID::ARST_VALUE, model::RSTVAL0, model::RSTVAL1 }
  };
  for (const auto &value: boolValues) {
    if (value.name == parName) {
      return value.get(cell.getParam(parName).as_bool());
    }
  }
  for (const auto &value: multibitValues) {
    if (value.name == parName) {
      return value.get(cell.getParam(parName)[portId] == RTLIL::S1);
    }
  }
  return 0;
}

struct SeqCellMapping {
  RTLIL::IdString type;
  model::CellSymbol symbol;
  RTLIL::IdString par1, par2, par3;

  model::CellSymbol mapCell(const RTLIL::Cell &cell, int portId) const {
    uint16_t mask1 = mapParameterValue(par1, cell, portId);
    uint16_t mask2 = mapParameterValue(par2, cell, portId);
    uint16_t mask3 = mapParameterValue(par3, cell, portId);
    return static_cast<model::CellSymbol>(symbol | mask1 | mask2 | mask3);
  }
};

static model::CellSymbol
mapSeqCellType(const RTLIL::Cell &cell, int portId) {
  static const SeqCellMapping mappings[] = {
    { ID($sdff), model::sDFF, SID::CLK_POLARITY, SID::SRST_POLARITY, SID::SRST_VALUE },
    { ID($adff), model::aDFF, SID::CLK_POLARITY, SID::ARST_POLARITY, SID::ARST_VALUE },
    { ID($dffsr), model::DFFrs, SID::CLK_POLARITY, SID::CLR_POLARITY, SID::SET_POLARITY },
    { ID($dff), model::DFF, SID::CLK_POLARITY },
    { ID($dlatch), model::DLATCH, SID::EN_POLARITY },
    { ID($adlatch), model::aDLATCH, SID::EN_POLARITY, SID::ARST_POLARITY, SID::ARST_VALUE },
    { ID($dlatchsr), model::DLATCHrs, SID::EN_POLARITY, SID::CLR_POLARITY, SID::SET_POLARITY },
    { ID($sr), model::LATCHrs, SID::CLR_POLARITY, SID::SET_POLARITY }
  };
  for (const SeqCellMapping &mapping: mappings) {
    if (mapping.type == cell.type) {
      return mapping.mapCell(cell, portId);
    }
  }
  return model::UNDEF;
}

struct SeqCellPorts {
  uint16_t nports;
  uint16_t width;
  std::vector<RTLIL::SigBit> ports[5]; // TODO replace with std::span

  /** @brief Store a single-bit slice of multibit ports to `links`.
   *  Single-bit ports are treated as multibit with a single driver for
   *  all bits.
   */
  void slice(std::vector<RTLIL::SigBit> &links, int bitN) const {
    assert(bitN >= 0);

    for (int i = 0; i < nports; ++i) {
      const std::vector<RTLIL::SigBit> &port = ports[i];
      const int index = (port.size() == 1) ? 0 : bitN;
      assert(static_cast<size_t>(index) < port.size());

      links.push_back(port[index]);
    }
  }
};

/** @brief Store a named port connection into the `conns` bitwise if
 *  the port is present.
 *
 *  @return `true` when the port is connected, `false` otherwise.
 */
static bool
getPortConnections(
    std::vector<RTLIL::SigBit> &conns,
    const RTLIL::IdString &portName,
    const RTLIL::Cell &cell) {
  if (cell.hasPort(portName)) {
    conns = cell.getPort(portName).bits();
    return true;
  }
  return false;
}

/** @brief Store port connections for a sequential cell in the following
 *  order: D, CLK/EN, RST, SET. Assumes all ports of a cell are connected.
 *  Ignores EN signal when CLK is present.
 */
static void
getSeqCellPorts(SeqCellPorts &info, const RTLIL::Cell &cell) {
  int nports = 0;
  nports += getPortConnections(info.ports[nports], SID::D, cell);
  nports += getPortConnections(info.ports[nports], SID::CLK, cell);

  // HACK Read EN signal for latches only since FFs with EN are not
  // supported
  if (!cell.hasParam(SID::CLK)) {
    nports += getPortConnections(info.ports[nports], SID::EN, cell);
  }

  // NOTE Only one of the RST signals can be found 
  nports += getPortConnections(info.ports[nports], SID::SRST, cell);
  nports += getPortConnections(info.ports[nports], SID::ARST, cell);
  nports += getPortConnections(info.ports[nports], SID::CLR, cell);

  nports += getPortConnections(info.ports[nports], SID::SET, cell);

  info.nports = nports;
  info.width = getIntPar(SID::WIDTH, cell);
}

void
ModuleBuilder::allocateSeqCell(
    model::NetBuilder &builder, const RTLIL::Cell &cell) {
  SeqCellPorts ports;
  getSeqCellPorts(ports, cell);
  const std::vector<LinkEnd> links(ports.nports);

  int portId = 0;
  for (RTLIL::SigBit bit: cell.getPort(SID::Q).bits()) {
    model::CellSymbol sym = mapSeqCellType(cell, portId);
    model::CellID cellId = model::makeCell(sym, links);

    builder.addCell(cellId);
    drivingPorts.emplace_back(bit, cellId);

    ports.slice(bitBuffer, portId);
    allocateDrivenPorts(bitBuffer, cellId);
    bitBuffer.clear();

    ++portId;
  }
}

struct CombCellMapping {
  RTLIL::IdString type;
  model::CellSymbol sym1, sym2;

  enum Kind {
    Same,
    Bitwise,
    Signed
  };

  Kind getAlternativeKind() const {
    if (sym1 == sym2) {
      return Same;
    }
    if (sym2 >= model::BNOT && sym2 <= model::BXNOR) {
      return Bitwise;
    }
    return Signed;
  }

  CombCellMapping(
      const RTLIL::IdString &type, model::CellSymbol sym1, model::CellSymbol sym2)
      : type(type), sym1(sym1), sym2(sym2)
  {}

  CombCellMapping(
      const RTLIL::IdString &type, model::CellSymbol sym)
      : CombCellMapping(type, sym, sym)
  {}

  COPY_MOVE(CombCellMapping);

  bool operator<(const CombCellMapping &that) const {
    return this->type < that.type;
  }

  bool operator<(const RTLIL::IdString &type) {
    return this->type < type;
  }
};

static bool
isSigned(const RTLIL::Cell &cell) {
  return getIntPar(SID::A_SIGNED, cell)
      && getIntPar(SID::B_SIGNED, cell);
}

CellTypeInstance::CellTypeInstance(const RTLIL::Cell &cell) {
  using namespace eda::gate::model;

  static CombCellMapping mappings[] = {
    { ID($not), NOT, BNOT },
    { ID($neg), NEG },
    { ID($pos), BUF },
    { ID($reduce_and), RAND },
    { ID($reduce_or), ROR },
    { ID($reduce_xor), RXOR },
    { ID($reduce_xnor), RXNOR },
    { ID($reduce_bool), ROR },
    { ID($and), AND, BAND },
    { ID($or), OR, BOR },
    { ID($xor), XOR, BXOR },
    { ID($xnor), XNOR, BXNOR },
    { ID($shl), SHL },
    { ID($shr), SHRu },
    { ID($sshl), SHL },
    { ID($sshr), SHRs },
    // { ID$($shift), SHRu, SHL }, // FIXME incorrect translation
    { ID($logic_not), RNOR },
    // { ID($logic_or), ROR },
    // ID($logic_and) = ($and ($reduce_or X) ($reduce_or Y))
    { ID($eqx), EQXu, EQXs },
    { ID($nex), NEQXu, NEQXs },
    { ID($lt), LTu, LTs },
    { ID($le), LTEu, LTEs },
    { ID($eq), EQu, EQs },
    { ID($ne), NEQu, NEQs },
    { ID($gt), GTu, GTs },
    { ID($ge), GTEu, GTEs },
    { ID($add), ADD },
    { ID($sub), SUB },
    { ID($mul), MULu, MULs },
    { ID($div), DIVu, DIVs },
    { ID($mod), REMs },
    { ID($mux), MUX2 },
    { ID($ternary), MUX2 }
  };
  static bool sorted = false;
  if (!sorted) {
    std::sort(std::begin(mappings), std::end(mappings));
    sorted = true;
  }
  const auto *it = std::lower_bound(
      std::begin(mappings), std::end(mappings), cell.type);
  if (it->type != cell.type) {
    LOG_FMT(ERROR, "rtlil", "{}: Unsupported cell type {} ({})",
        cell.get_src_attribute(), cell.type, cell.name);
    kind = UNDEF;
    widthY = 0;
    widthA = 0;
    widthB = 0;
    return;
  }

  if (cell.hasParam(SID::WIDTH)) {
    widthY = getIntPar(SID::WIDTH, cell);
    widthA = 0;
    widthB = 0;
  } else {
    widthY = getIntPar(SID::Y_WIDTH, cell);
    widthA = getIntPar(SID::A_WIDTH, cell);
    widthB = getIntPar(SID::B_WIDTH, cell);
  }

  switch (it->getAlternativeKind()) {
  case CombCellMapping::Same:
    kind = it->sym1;
    break;

  case CombCellMapping::Bitwise:
    kind = (widthA > 1 && widthB > 1) ? it->sym2 : it->sym1;
    break;

  case CombCellMapping::Signed:
    kind = isSigned(cell) ? it->sym2 : it->sym1;
    break;
  }
}

void
ModuleBuilder::allocateCombCell(
    model::NetBuilder &builder, const RTLIL::Cell &cell) {
  CellTypeInstance inst(cell);
  if (!inst.isValid()) {
    return;
  }
  model::CellTypeID typeId = ctx.getInstanceCellTypeID(inst, cell.type);
  model::CellID cellId =
      allocateCell(builder, &cell, typeId, inst.getNumInputPorts());
  connectDrivingPorts(cellId, cell.getPort(SID::Y).bits());
}

static std::string
makeReadVerilogCmd(const std::vector<std::string> &ss) {
  size_t total = 0;
  for (const std::string &s: ss) {
    total += s.size();
  }
  total += ss.size();

  std::string cmd = "read_verilog";
  cmd.reserve(total + cmd.size());
  for (const std::string &s: ss) {
    cmd += ' ';
    cmd += s;
  }
  return cmd;
}

static int
readVerilogDesign(
    RTLIL::Design &design,
    const std::vector<std::string> &files) {
  assert(design.modules_.empty() && "Input design is not empty");

  Yosys::yosys_setup();
  Yosys::run_pass("design -reset-vlog", &design);

  std::string readVerilog = makeReadVerilogCmd(files);
  Yosys::run_pass(readVerilog.c_str(), &design);

  return static_cast<int>(design.modules_.size());
}

namespace eda::gate::translator {

model::NetID
readVerilogDesign(
    const std::string &top, const std::vector<std::string> &files) {
  RTLIL::Design design;
  const int nmodules = ::readVerilogDesign(design, files);
  if (!nmodules) {
    return model::OBJ_NULL_ID;
  }

  if (!top.empty() && hasModule(top, design)) {
    Yosys::run_pass("hierarchy -top " + top, &design);
  } else {
    Yosys::run_pass("hierarchy -auto-top", &design);
  }
  Yosys::run_pass("proc", &design);
  Yosys::run_pass("opt -nodffe", &design);
  Yosys::run_pass("memory", &design);
  Yosys::run_pass("pmuxtree", &design);
  Yosys::run_pass("flatten -noscopeinfo", &design);
  Yosys::run_pass("opt -nodffe -fast", &design);
  // Yosys::run_pass("splitnets -ports", &design);

  if (!design.top_module()) {
    return model::OBJ_NULL_ID;
  }
  DesignBuilder builder;
  builder.translateDesign(design);

  model::CellTypeID typeId =
      builder.getModuleType(*design.top_module()).typeId;
  return model::CellType::get(typeId).getImpl();
}

} // namespace eda::gate::translator
