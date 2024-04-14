FIRRTL version 3.2.0
circuit adder :
  module adder :
    output sum_out : UInt<4>
    output carry_out : UInt<1>
    input carry_in : UInt<1>
    input ina : UInt<4>
    input inb : UInt<4>
    wire _GEN__2 : UInt
    wire _GEN__1 : UInt<5>
    wire _GEN__0 : UInt<5>
    _GEN__2 <= add(bits(_GEN__0, 4, 0), bits(carry_in, 0, 0))
    _GEN__1 <= add(bits(ina, 3, 0), bits(inb, 3, 0))
    sum_out <= bits(_GEN__2, 4, 1)
    carry_out <= bits(_GEN__2, 0, 0)
    _GEN__0 <= bits(_GEN__1, 4, 0)
