FIRRTL version 3.2.0
circuit picorv :
  module picorv :
    output Q : UInt<1>
    input CLK : UInt<1>
    input D : UInt<1>
    input R : UInt<1>
    input D1 : UInt<1>
    wire _GEN_2_ : Clock
    wire _GEN_0_ : UInt<1>
    regreset _GEN_1_ : UInt<1>, _GEN_2_, R, UInt(0b0)
    _GEN_1_ <= UInt<32>(0b00000000000000000000001000001001)
    Q <= _GEN_0_
    _GEN_0_ <= bits(_GEN_1_, 0, 0)
    _GEN_2_ <= asClock(CLK)
