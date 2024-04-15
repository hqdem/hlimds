FIRRTL version 3.2.0
circuit gray2bin1 :
  module gray2bin1 :
    output bin : UInt<8>
    input gray : UInt<8>
    wire _GEN__14 : UInt
    wire _GEN__13 : UInt
    wire _GEN__12 : UInt
    wire _GEN__11 : UInt
    wire _GEN__10 : UInt
    wire _GEN__9 : UInt
    wire _GEN__8 : UInt
    wire _GEN__7 : UInt<1>
    wire _GEN__6 : UInt<1>
    wire _GEN__5 : UInt<1>
    wire _GEN__4 : UInt<1>
    wire _GEN__3 : UInt<1>
    wire _GEN__2 : UInt<1>
    wire _GEN__1 : UInt<1>
    wire _GEN__0 : UInt<1>
    _GEN__14 <= cat(_GEN__13, bits(_GEN__0, 0, 0))
    _GEN__13 <= cat(_GEN__12, bits(_GEN__1, 0, 0))
    _GEN__12 <= cat(_GEN__11, bits(_GEN__2, 0, 0))
    _GEN__11 <= cat(_GEN__10, bits(_GEN__3, 0, 0))
    _GEN__10 <= cat(_GEN__9, bits(_GEN__4, 0, 0))
    _GEN__9 <= cat(_GEN__8, bits(_GEN__5, 0, 0))
    _GEN__8 <= cat(bits(_GEN__7, 0, 0), bits(_GEN__6, 0, 0))
    _GEN__7 <= bits(gray, 7, 7)
    _GEN__6 <= xorr(bits(gray, 7, 6))
    _GEN__5 <= xorr(bits(gray, 7, 5))
    _GEN__4 <= xorr(bits(gray, 7, 4))
    _GEN__3 <= xorr(bits(gray, 7, 3))
    _GEN__2 <= xorr(bits(gray, 7, 2))
    _GEN__1 <= xorr(bits(gray, 7, 1))
    _GEN__0 <= xorr(bits(gray, 7, 0))
    bin <= _GEN__14
