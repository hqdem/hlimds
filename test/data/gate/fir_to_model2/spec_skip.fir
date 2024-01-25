FIRRTL version 3.2.0
  circuit SkipStatement:
    module SkipStatement:
      input in: UInt<1>
      output out: UInt<1>
      skip
      connect out, in
