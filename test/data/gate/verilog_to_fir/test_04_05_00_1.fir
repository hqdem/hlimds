FIRRTL version 3.2.0
circuit test :
  module test :
    input a : UInt<8>
    input b : UInt<8>
    input c : UInt<8>
    input d : UInt<8>
    output e : UInt<8>
    output f : UInt<8>
    output g : UInt<8>
    output h : UInt<8>
    wire _GEN__7 : UInt
    wire _GEN__6 : UInt
    wire _GEN__5 : UInt
    wire _GEN__4 : UInt
    wire _GEN__3 : UInt<8>
    wire _GEN__2 : UInt<8>
    wire _GEN__1 : UInt<8>
    wire _GEN__0 : UInt<8>
    _GEN__7 <= _GEN__6
    _GEN__6 <= cat(_GEN__5, bits(a, 1, 0))
    _GEN__5 <= cat(_GEN__4, bits(b, 3, 2))
    _GEN__4 <= cat(bits(c, 7, 6), bits(d, 5, 4))
    _GEN__3 <= _GEN__7
    _GEN__2 <= sub(bits(c, 7, 0), bits(d, 7, 0))
    _GEN__1 <= dshr(bits(h, 7, 0), bits(e, 3, 0))
    _GEN__0 <= add(bits(a, 7, 0), bits(b, 7, 0))
    e <= bits(_GEN__0, 7, 0)
    f <= bits(_GEN__3, 7, 0)
    g <= bits(_GEN__1, 7, 0)
    h <= bits(_GEN__2, 7, 0)
