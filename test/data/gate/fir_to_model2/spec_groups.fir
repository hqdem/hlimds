FIRRTL version 3.2.0
  circuit Foo:
    declgroup Bar, bind: ; Declaration of group Bar with convention "bind"
    module Foo:
      input a: UInt<1>
    group Bar:
      node notA = not(a)
