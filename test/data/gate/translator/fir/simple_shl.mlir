firrtl.circuit "SimpleShl" {
firrtl.module @SimpleShl(in %a : !firrtl.uint<10>, out %res : !firrtl.uint<12>) {
  %1 = firrtl.shl %a, 2 : (!firrtl.uint<10>) -> !firrtl.uint<12>
  firrtl.connect %res, %1 : !firrtl.uint<12>, !firrtl.uint<12>
}
}
