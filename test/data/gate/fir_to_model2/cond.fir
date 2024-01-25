FIRRTL version 3.2.0
circuit picorv :
  module picorv :
    input sel : UInt<2>
    output out : UInt<4>
    wire _GEN_17_ : UInt<4>
    wire _GEN_16_ : UInt<4>
    wire _GEN_15_ : UInt<1>
    wire _GEN_14_ : UInt<1>
    wire _GEN_13_ : UInt<1>
    wire _GEN_12_ : UInt<1>
    wire _GEN_11_ : UInt<4>
    wire _GEN_10_ : UInt<4>
    wire _GEN_9_ : UInt<4>
    wire _GEN_8_ : UInt<1>
    wire out_reg : UInt<4>
    wire _GEN_2_ : UInt<4>
    invalidate _GEN_2_
    wire _GEN_3_ : UInt<4>
    invalidate _GEN_3_
    wire _GEN_4_ : UInt<1>
    wire _GEN_5_ : UInt<1>
    wire _GEN_6_ : UInt<1>
    wire _GEN_7_ : UInt<1>
    wire _GEN_1_ : UInt<1>
    wire _GEN_0_ : UInt<4>
    _GEN_17_ <= _GEN_11_
    _GEN_16_ <= bits(out, 3, 0)
    _GEN_15_ <= not(bits(sel, 1, 0))
    _GEN_14_ <= eq(bits(sel, 1, 0), UInt<2>(0b01))
    _GEN_13_ <= eq(bits(sel, 1, 0), UInt<2>(0b10))
    _GEN_12_ <= eq(bits(sel, 1, 0), UInt<2>(0b11))
    _GEN_11_ <= mux(bits(_GEN_1_, 0, 0), bits(_GEN_3_, 3, 0), bits(_GEN_2_, 3, 0))
    _GEN_10_ <= mux(bits(_GEN_4_, 0, 0), UInt<4>(0b1000), UInt<4>(0b0100))
    _GEN_9_ <= mux(bits(_GEN_6_, 0, 0), UInt<4>(0b0010), UInt<4>(0b0001))
    _GEN_8_ <= or(bits(_GEN_5_, 0, 0), bits(_GEN_4_, 0, 0))
    out <= bits(_GEN_17_, 3, 0)
    out_reg <= bits(_GEN_16_, 3, 0)
    _GEN_2_ <= bits(_GEN_9_, 3, 0)
    _GEN_3_ <= bits(_GEN_10_, 3, 0)
    _GEN_4_ <= bits(_GEN_12_, 0, 0)
    _GEN_5_ <= bits(_GEN_13_, 0, 0)
    _GEN_6_ <= bits(_GEN_14_, 0, 0)
    _GEN_7_ <= bits(_GEN_15_, 0, 0)
    _GEN_1_ <= bits(_GEN_8_, 0, 0)
    _GEN_0_ <= bits(_GEN_11_, 3, 0)
