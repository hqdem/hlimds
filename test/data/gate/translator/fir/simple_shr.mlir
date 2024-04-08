firrtl.circuit "SimpleShr" {
firrtl.module @SimpleShr(in %a : !firrtl.uint<10>, out %res : !firrtl.uint<8>) {
  %1 = firrtl.shr %a, 2 : (!firrtl.uint<10>) -> !firrtl.uint<8>
  firrtl.connect %res, %1 : !firrtl.uint<8>, !firrtl.uint<8>
}
}
