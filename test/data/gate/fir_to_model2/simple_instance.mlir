firrtl.circuit "SimpleInstance" {
firrtl.extmodule @ExternalAddModule(in a : !firrtl.uint<10>, in b: !firrtl.uint<10>, out sum : !firrtl.uint<10>)
firrtl.module @SimpleInstance(in %a : !firrtl.uint<10>, in %b: !firrtl.uint<10>, out %res : !firrtl.uint<10>) {
  %1, %2, %3 = firrtl.instance "ExternalAddInstance" @ExternalAddModule(in a: !firrtl.uint<10>, in b: !firrtl.uint<10>, out sum: !firrtl.uint<10>)
  firrtl.connect %1, %a : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %2, %b : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %res, %3 : !firrtl.uint<10>, !firrtl.uint<10>
}
}
