firrtl.circuit "AddInstanceMix" {
firrtl.extmodule @ExternalAddModule(in a : !firrtl.uint<11>, in b: !firrtl.uint<11>, out sum : !firrtl.uint<11>)
firrtl.module @AddInstanceMix(in %a : !firrtl.uint<10>, in %b: !firrtl.uint<10>, in %c : !firrtl.uint<10>, in %d: !firrtl.uint<10>, out %sum : !firrtl.uint<11>) {
  %1 = firrtl.add %a, %b : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<11>
  %2 = firrtl.add %c, %d : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<11>
  %3, %4, %5 = firrtl.instance "ExternalAddInstance" @ExternalAddModule(in a: !firrtl.uint<11>, in b: !firrtl.uint<11>, out sum: !firrtl.uint<11>)
  firrtl.connect %3, %1 : !firrtl.uint<11>, !firrtl.uint<11>
  firrtl.connect %4, %2 : !firrtl.uint<11>, !firrtl.uint<11>
  firrtl.connect %sum, %5 : !firrtl.uint<11>, !firrtl.uint<11>
}
}
