FIRRTL version 3.2.0
  circuit Foo:
    module NoSubAccessesWithProbes :
      input x : {a : Probe<UInt<1>[2]>, b : UInt<1>}[3]
      input i : UInt<1>
      input c : const UInt<1>
      output p : Probe<UInt<1>>
      ; Illegal: x[i], x[c]
      ; Illegal: x[0].a[i], x[0].a[c]
      ; Legal:
      define p = x[0].a[1]
    module Foo:
