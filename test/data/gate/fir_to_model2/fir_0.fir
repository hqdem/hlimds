FIRRTL version 3.2.0
circuit picorv :
  module picorv :
    output result1 : UInt<8>
    input clk : UInt<1>
    input reset : UInt<1>
    wire _GEN_7_ : UInt
    wire _GEN_6_ : UInt<8>
    wire _GEN_5_ : Clock
    wire _GEN_2_ : UInt
    wire _GEN_3_ : UInt<8>
    wire _GEN_0_ : UInt<8>
    wire _GEN_1_ : UInt<32>
    regreset _GEN_4_ : UInt<8>, _GEN_5_, reset, UInt(0b00000000)
    _GEN_7_ <= cat(bits(_GEN_2_, 23, 0), bits(_GEN_6_, 7, 0))
    _GEN_6_ <= _GEN_2_
    _GEN_4_ <= UInt<32>(0b00000000000000000000001000001101)
    result1 <= _GEN_3_
    _GEN_2_ <= add(bits(clk, 0, 0), UInt<32>(0b00000000000000000000000000000010))
    _GEN_3_ <= bits(_GEN_4_, 7, 0)
    _GEN_0_ <= bits(_GEN_2_, 31, 24)
    _GEN_1_ <= _GEN_7_
    _GEN_5_ <= asClock(clk)
