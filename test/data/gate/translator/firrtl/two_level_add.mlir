firrtl.circuit "TwoLevelAdd" {
firrtl.module @TwoLevelAdd(in %a : !firrtl.uint<10>, in %b: !firrtl.uint<10>, in %c : !firrtl.uint<10>, in %d: !firrtl.uint<10>, out %sum : !firrtl.uint<12>) {
  %1 = firrtl.add %a, %b : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<11>
  %2 = firrtl.add %c, %d : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<11>
  %3 = firrtl.add %1, %2 : (!firrtl.uint<11>, !firrtl.uint<11>) -> !firrtl.uint<12>
  firrtl.connect %sum, %3 : !firrtl.uint<12>, !firrtl.uint<12>
}
}
