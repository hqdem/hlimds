FIRRTL version 3.2.0
circuit picorv :
  module picorv :
    output out : UInt<1>
    input a : UInt<1>
    input sel : UInt<1>
    input in0 : UInt<1>
    input in1 : UInt<1>
    wire _GEN_8_ : Clock
    wire _GEN_5_ : UInt<1>
    wire _GEN_4_ : UInt<1>
    wire _GEN_3_ : UInt<1>
    wire _GEN_6_ : UInt<1>
    wire _GEN_0_ : UInt<1>
    wire _GEN_2_ : UInt<1>
    wire _GEN_1_ : UInt<1>
    reg _GEN_7_ : UInt<1>, _GEN_8_
    _GEN_7_ <= bits(_GEN_0_, 0, 0)
    out <= _GEN_6_
    _GEN_5_ <= or(bits(_GEN_2_, 0, 0), bits(_GEN_1_, 0, 0))
    _GEN_4_ <= and(bits(sel, 0, 0), bits(in1, 0, 0))
    _GEN_3_ <= add(bits(in0, 0, 0), bits(in1, 0, 0))
    _GEN_6_ <= bits(_GEN_7_, 0, 0)
    _GEN_0_ <= bits(_GEN_5_, 0, 0)
    _GEN_2_ <= bits(_GEN_4_, 0, 0)
    _GEN_1_ <= bits(_GEN_3_, 0, 0)
    _GEN_8_ <= asClock(a)
