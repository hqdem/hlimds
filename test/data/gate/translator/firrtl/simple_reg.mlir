firrtl.circuit "SimpleReg" {
firrtl.module @SimpleReg(in %a : !firrtl.uint<10>, in %clock : !firrtl.clock, out %res : !firrtl.uint<10>) {
  %r = firrtl.reg %clock : !firrtl.clock, !firrtl.uint<10>
  firrtl.connect %r, %a : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %res, %r : !firrtl.uint<10>, !firrtl.uint<10>
}
}
