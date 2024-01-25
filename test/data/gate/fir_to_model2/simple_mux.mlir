firrtl.circuit "SimpleMux" {
firrtl.module @SimpleMux(in %a : !firrtl.uint<1>, in %b: !firrtl.uint<1>, in %s: !firrtl.uint<1>, out %res : !firrtl.uint<1>) {
  %1 = firrtl.mux (%a, %b, %s) : (!firrtl.uint<1>, !firrtl.uint<1>, !firrtl.uint<1>) -> !firrtl.uint<1>
  firrtl.connect %res, %1 : !firrtl.uint<1>, !firrtl.uint<1>
}
}
