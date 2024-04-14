FIRRTL version 3.2.0
circuit c17 :
  module c17 :
    input N1 : UInt<1>
    input N2 : UInt<1>
    input N3 : UInt<1>
    input N6 : UInt<1>
    input N7 : UInt<1>
    output N22 : UInt<1>
    output N23 : UInt<1>
    wire _GEN__17 : UInt<1>
    wire _GEN__16 : UInt<1>
    wire _GEN__15 : UInt<1>
    wire _GEN__14 : UInt<1>
    wire _GEN__13 : UInt<1>
    wire _GEN__12 : UInt<1>
    wire _GEN__11 : UInt<1>
    wire _GEN__10 : UInt<1>
    wire _GEN__9 : UInt<1>
    wire _GEN__8 : UInt<1>
    wire _GEN__7 : UInt<1>
    wire _GEN__6 : UInt<1>
    wire _GEN__5 : UInt<1>
    wire _GEN__4 : UInt<1>
    wire _GEN__3 : UInt<1>
    wire _GEN__2 : UInt<1>
    wire _GEN__1 : UInt<1>
    wire _GEN__0 : UInt<1>
    wire N19 : UInt<1>
    wire N16 : UInt<1>
    wire N11 : UInt<1>
    wire N10 : UInt<1>
    _GEN__17 <= not(bits(_GEN__5, 0, 0))
    _GEN__16 <= not(bits(_GEN__4, 0, 0))
    _GEN__15 <= not(bits(_GEN__3, 0, 0))
    _GEN__14 <= not(bits(_GEN__2, 0, 0))
    _GEN__13 <= not(bits(_GEN__1, 0, 0))
    _GEN__12 <= not(bits(_GEN__0, 0, 0))
    _GEN__11 <= and(bits(N16, 0, 0), bits(N19, 0, 0))
    _GEN__10 <= and(bits(N10, 0, 0), bits(N16, 0, 0))
    _GEN__9 <= and(bits(N11, 0, 0), bits(N7, 0, 0))
    _GEN__8 <= and(bits(N2, 0, 0), bits(N11, 0, 0))
    _GEN__7 <= and(bits(N3, 0, 0), bits(N6, 0, 0))
    _GEN__6 <= and(bits(N1, 0, 0), bits(N3, 0, 0))
    _GEN__5 <= bits(_GEN__11, 0, 0)
    _GEN__4 <= bits(_GEN__10, 0, 0)
    _GEN__3 <= bits(_GEN__9, 0, 0)
    _GEN__2 <= bits(_GEN__8, 0, 0)
    _GEN__1 <= bits(_GEN__7, 0, 0)
    _GEN__0 <= bits(_GEN__6, 0, 0)
    N19 <= bits(_GEN__15, 0, 0)
    N16 <= bits(_GEN__14, 0, 0)
    N11 <= bits(_GEN__13, 0, 0)
    N10 <= bits(_GEN__12, 0, 0)
    N23 <= bits(_GEN__17, 0, 0)
    N22 <= bits(_GEN__16, 0, 0)
