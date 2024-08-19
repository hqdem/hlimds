firrtl.circuit "SimpleAdd" {
firrtl.module @SimpleAdd(in %a : !firrtl.uint<10>, in %b: !firrtl.uint<10>, out %sum : !firrtl.uint<11>) {
  %1 = firrtl.add %a, %b : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<11>
  firrtl.connect %sum, %1 : !firrtl.uint<11>, !firrtl.uint<11>
}
}
