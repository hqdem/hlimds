firrtl.circuit "TwoLevelXor" {
firrtl.module @TwoLevelXor(in %a : !firrtl.uint<10>, in %b: !firrtl.uint<10>, in %c : !firrtl.uint<10>, in %d: !firrtl.uint<10>, out %res : !firrtl.uint<10>) {
  %1 = firrtl.xor %a, %b : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<10>
  %2 = firrtl.xor %c, %d : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<10>
  %3 = firrtl.xor %1, %2 : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<10>
  firrtl.connect %res, %3 : !firrtl.uint<10>, !firrtl.uint<10>
}
}
