firrtl.circuit "TwoLevelInstance" {
firrtl.extmodule @ExternalAddModule(in a : !firrtl.uint<10>, in b: !firrtl.uint<10>, out sum : !firrtl.uint<10>)
firrtl.module @TwoLevelInstance(in %a : !firrtl.uint<10>, in %b: !firrtl.uint<10>, out %res : !firrtl.uint<10>) {
  %1, %2, %3 = firrtl.instance "ExternalAddInstance1" @ExternalAddModule(in a: !firrtl.uint<10>, in b: !firrtl.uint<10>, out sum: !firrtl.uint<10>)
  %4, %5, %6 = firrtl.instance "ExternalAddInstance2" @ExternalAddModule(in a: !firrtl.uint<10>, in b: !firrtl.uint<10>, out sum: !firrtl.uint<10>)
  %7, %8, %9 = firrtl.instance "ExternalAddInstance3" @ExternalAddModule(in a: !firrtl.uint<10>, in b: !firrtl.uint<10>, out sum: !firrtl.uint<10>)
  firrtl.connect %1, %a : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %2, %b : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %7, %3 : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %4, %a : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %5, %b : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %8, %6 : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %res, %9 : !firrtl.uint<10>, !firrtl.uint<10>
}
}
