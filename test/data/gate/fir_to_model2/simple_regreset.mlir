firrtl.circuit "SimpleRegReset" {
firrtl.module @SimpleRegReset(in %a : !firrtl.uint<10>, in %clock : !firrtl.clock, in %resetSignal : !firrtl.reset, in %resetValue : !firrtl.uint<10>, out %res : !firrtl.uint<10>) {
  %r = firrtl.regreset %clock, %resetSignal, %resetValue : !firrtl.clock, !firrtl.reset, !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %r, %a : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %res, %r : !firrtl.uint<10>, !firrtl.uint<10>
}
}
