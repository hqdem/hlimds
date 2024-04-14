FIRRTL version 3.2.0
circuit ffnand_wave :
  module ffnand :
    output q : UInt<1>
    output qbar : UInt<1>
    input preset : UInt<1>
    input clear : UInt<1>
    wire _GEN__5 : UInt<1>
    wire _GEN__4 : UInt<1>
    wire _GEN__3 : UInt<1>
    wire _GEN__2 : UInt<1>
    wire _GEN__0 : UInt<1>
    wire _GEN__1 : UInt<1>
    _GEN__5 <= not(bits(_GEN__1, 0, 0))
    _GEN__4 <= not(bits(_GEN__0, 0, 0))
    _GEN__3 <= and(bits(q, 0, 0), bits(clear, 0, 0))
    _GEN__2 <= and(bits(qbar, 0, 0), bits(preset, 0, 0))
    q <= bits(_GEN__4, 0, 0)
    _GEN__0 <= bits(_GEN__2, 0, 0)
    _GEN__1 <= bits(_GEN__3, 0, 0)
    qbar <= bits(_GEN__5, 0, 0)
  module ffnand_wave :
    wire _GEN__7 : UInt<1>
    wire _GEN__6 : UInt<1>
    wire in2 : UInt<1>
    wire in1 : UInt<1>
    _GEN__7 <= UInt<1>(0b1)
    _GEN__6 <= UInt<1>(0b1)
    in2 <= bits(_GEN__7, 0, 0)
    in1 <= bits(_GEN__6, 0, 0)
