FIRRTL version 3.2.0
  circuit Foo:
    extmodule MyExternalModule :
      input foo: UInt<2>
      output bar: UInt<4>
      output baz: SInt<8>
      defname = VerilogName
      parameter x = "hello"
      parameter y = 42
    module Foo:
