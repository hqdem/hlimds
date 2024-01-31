firrtl.circuit "SimpleXor" {
firrtl.module @SimpleXor(in %a : !firrtl.uint<10>, in %b: !firrtl.uint<10>, out %res : !firrtl.uint<10>) {
  %1 = firrtl.xor %a, %b : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<10>
  firrtl.connect %res, %1 : !firrtl.uint<10>, !firrtl.uint<10>
}
}
