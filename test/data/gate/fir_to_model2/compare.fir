FIRRTL version 3.2.0
circuit picorv :
  module picorv :
    input x : UInt<1>
    output y : UInt<1>
    wire _GEN_0_ : UInt<1>
    _GEN_0_ <= mux(bits(x, 0, 0), UInt<1>(0b1), UInt<1>(0b0))
    y <= bits(_GEN_0_, 0, 0)
