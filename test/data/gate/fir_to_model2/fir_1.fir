FIRRTL version 3.2.0
circuit picorv :
  module picorv :
    input a : UInt<64>
    input b : UInt<64>
    output y : UInt<64>
    wire _GEN_0_ : UInt<64>
    _GEN_0_ <= and(bits(a, 63, 0), bits(b, 63, 0))
    y <= bits(_GEN_0_, 63, 0)
