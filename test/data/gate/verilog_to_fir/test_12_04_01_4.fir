FIRRTL version 3.2.0
circuit addergen1 :
  module addergen1 :
    output co : UInt<1>
    output sum : UInt<4>
    input a : UInt<4>
    input b : UInt<4>
    input ci : UInt<1>
    wire _GEN__41 : UInt
    wire _GEN__40 : UInt
    wire _GEN__39 : UInt
    wire _GEN__38 : UInt
    wire _GEN__37 : UInt
    wire _GEN__36 : UInt
    wire _GEN__35 : UInt
    wire _GEN__34 : UInt
    wire _GEN__33 : UInt
    wire _GEN__32 : UInt
    wire _GEN__31 : UInt<1>
    wire _GEN__30 : UInt<1>
    wire _GEN__29 : UInt<1>
    wire _GEN__28 : UInt<1>
    wire _GEN__27 : UInt<1>
    wire _GEN__26 : UInt<1>
    wire _GEN__25 : UInt<1>
    wire _GEN__24 : UInt<1>
    wire _GEN__23 : UInt<1>
    wire _GEN__22 : UInt<1>
    wire _GEN__21 : UInt<1>
    wire _GEN__20 : UInt<1>
    wire _GEN__19 : UInt<1>
    wire _GEN__18 : UInt<1>
    wire _GEN__17 : UInt<1>
    wire _GEN__16 : UInt<1>
    wire _GEN__15 : UInt<1>
    wire _GEN__14 : UInt<1>
    wire _GEN__13 : UInt<1>
    wire _GEN__12 : UInt<1>
    wire _GEN__6 : UInt<1>
    wire _GEN__3 : UInt<1>
    wire _GEN__0 : UInt<1>
    wire _GEN__1 : UInt<1>
    wire _GEN__10 : UInt<1>
    wire _GEN__9 : UInt<1>
    wire _GEN__5 : UInt<1>
    wire _GEN__8 : UInt<1>
    wire _GEN__4 : UInt<1>
    wire _GEN__7 : UInt<1>
    wire _GEN__11 : UInt<1>
    wire _GEN__2 : UInt<1>
    wire c : UInt<5>
    _GEN__41 <= cat(_GEN__40, bits(_GEN__32, 1, 1))
    _GEN__40 <= cat(_GEN__39, bits(_GEN__20, 0, 0))
    _GEN__39 <= cat(_GEN__38, bits(_GEN__21, 0, 0))
    _GEN__38 <= cat(bits(_GEN__32, 0, 0), bits(_GEN__22, 0, 0))
    _GEN__37 <= cat(_GEN__36, bits(_GEN__28, 0, 0))
    _GEN__36 <= cat(_GEN__35, bits(_GEN__29, 0, 0))
    _GEN__35 <= cat(bits(_GEN__31, 0, 0), bits(_GEN__30, 0, 0))
    _GEN__34 <= _GEN__33
    _GEN__33 <= cat(_GEN__23, bits(ci, 0, 0))
    _GEN__32 <= _GEN__34
    _GEN__31 <= xor(bits(_GEN__9, 0, 0), bits(c, 3, 3))
    _GEN__30 <= xor(bits(_GEN__6, 0, 0), bits(c, 2, 2))
    _GEN__29 <= xor(bits(_GEN__3, 0, 0), bits(c, 1, 1))
    _GEN__28 <= xor(bits(_GEN__0, 0, 0), bits(ci, 0, 0))
    _GEN__27 <= xor(bits(a, 3, 3), bits(b, 3, 3))
    _GEN__26 <= xor(bits(a, 2, 2), bits(b, 2, 2))
    _GEN__25 <= xor(bits(a, 1, 1), bits(b, 1, 1))
    _GEN__24 <= xor(bits(a, 0, 0), bits(b, 0, 0))
    _GEN__23 <= or(bits(_GEN__10, 0, 0), bits(_GEN__11, 0, 0))
    _GEN__22 <= or(bits(_GEN__7, 0, 0), bits(_GEN__8, 0, 0))
    _GEN__21 <= or(bits(_GEN__4, 0, 0), bits(_GEN__5, 0, 0))
    _GEN__20 <= or(bits(_GEN__1, 0, 0), bits(_GEN__2, 0, 0))
    _GEN__19 <= and(bits(_GEN__9, 0, 0), bits(c, 3, 3))
    _GEN__18 <= and(bits(_GEN__6, 0, 0), bits(c, 2, 2))
    _GEN__17 <= and(bits(_GEN__3, 0, 0), bits(c, 1, 1))
    _GEN__16 <= and(bits(_GEN__0, 0, 0), bits(ci, 0, 0))
    _GEN__15 <= and(bits(a, 3, 3), bits(b, 3, 3))
    _GEN__14 <= and(bits(a, 2, 2), bits(b, 2, 2))
    _GEN__13 <= and(bits(a, 1, 1), bits(b, 1, 1))
    _GEN__12 <= and(bits(a, 0, 0), bits(b, 0, 0))
    _GEN__6 <= bits(_GEN__26, 0, 0)
    _GEN__3 <= bits(_GEN__25, 0, 0)
    _GEN__0 <= bits(_GEN__24, 0, 0)
    _GEN__1 <= bits(_GEN__12, 0, 0)
    sum <= _GEN__37
    _GEN__10 <= bits(_GEN__15, 0, 0)
    _GEN__9 <= bits(_GEN__27, 0, 0)
    _GEN__5 <= bits(_GEN__17, 0, 0)
    _GEN__8 <= bits(_GEN__18, 0, 0)
    _GEN__4 <= bits(_GEN__13, 0, 0)
    _GEN__7 <= bits(_GEN__14, 0, 0)
    _GEN__11 <= bits(_GEN__19, 0, 0)
    _GEN__2 <= bits(_GEN__16, 0, 0)
    c <= _GEN__41
    co <= bits(_GEN__23, 0, 0)
