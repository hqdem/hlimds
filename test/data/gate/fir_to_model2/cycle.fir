FIRRTL version 3.2.0
circuit picorv :
  module picorv :
    input clk : UInt<1>
    input rst : UInt<1>
    output count_out : UInt<8>
    wire _GEN_7_ : UInt
    wire _GEN_6_ : UInt<8>
    wire _GEN_5_ : UInt<8>
    wire _GEN_4_ : Clock
    wire _GEN_2_ : UInt
    wire count : UInt<8>
    wire _GEN_0_ : UInt<8>
    wire _GEN_1_ : UInt<32>
    regreset _GEN_3_ : UInt<8>, _GEN_4_, rst, UInt(0b00000000)
    _GEN_7_ <= cat(bits(_GEN_2_, 23, 0), bits(_GEN_5_, 7, 0))
    _GEN_6_ <= _GEN_3_
    _GEN_5_ <= _GEN_2_
    _GEN_3_ <= UInt<32>(0b00000000000000000000001000001110)
    _GEN_2_ <= add(bits(count, 7, 0), UInt<32>(0b00000000000000000000000000000001))
    count_out <= bits(_GEN_6_, 7, 0)
    count <= bits(_GEN_3_, 7, 0)
    _GEN_0_ <= bits(_GEN_2_, 31, 24)
    _GEN_1_ <= _GEN_7_
    _GEN_4_ <= asClock(clk)
