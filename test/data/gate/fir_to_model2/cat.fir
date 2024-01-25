FIRRTL version 3.2.0
circuit picorv :
  module picorv :
    input a : UInt<2>
    input b : UInt<1>
    input c : UInt<1>
    input d : UInt<1>
    output abc_vector : UInt<4>
    output abc_ : UInt<5>
    wire _GEN_5_ : UInt
    wire _GEN_4_ : UInt
    wire _GEN_3_ : UInt<4>
    wire _GEN_2_ : UInt
    wire _GEN_1_ : UInt
    wire _GEN_0_ : UInt<5>
    _GEN_5_ <= cat(UInt<2>(0b00), _GEN_4_)
    _GEN_4_ <= cat(bits(a, 1, 1), bits(b, 0, 0))
    _GEN_3_ <= _GEN_5_
    _GEN_2_ <= cat(bits(a, 1, 0), _GEN_1_)
    _GEN_1_ <= cat(UInt<2>(0b00), bits(a, 1, 1))
    _GEN_0_ <= _GEN_2_
    abc_vector <= bits(_GEN_3_, 3, 0)
    abc_ <= bits(_GEN_0_, 4, 0)
