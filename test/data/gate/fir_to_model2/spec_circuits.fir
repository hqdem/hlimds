FIRRTL version 3.2.0
circuit Baz:
  module Bar :
    input a: UInt<1>
    output b: UInt<1>
    connect b, a
  module Baz :
    input a: UInt<1>
    output b: UInt<1>
    inst bar of Bar
    connect bar.a, a
    connect b, bar.b
