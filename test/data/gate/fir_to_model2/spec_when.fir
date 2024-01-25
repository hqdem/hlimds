FIRRTL version 3.2.0
  circuit WhenConstruct :
    module WhenConstruct :
      input a: UInt<1>
      input b: UInt<1>
      input en: UInt<1>
      output x: UInt<1>
      when en :
        connect x, a
      else :
        connect x, b
