FIRRTL version 3.2.0
circuit XORgate :
  module XORgate :
    input a : UInt<1>
    input b : UInt<1>
    output out : UInt<1>
    wire _GEN__0 : UInt<1>
    _GEN__0 <= xor(bits(a, 0, 0), bits(b, 0, 0))
    out <= bits(_GEN__0, 0, 0)
