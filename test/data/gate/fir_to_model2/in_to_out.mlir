firrtl.circuit "InToOut" {
firrtl.module @InToOut(in %a : !firrtl.uint<10>, out %res : !firrtl.uint<10>) {
  firrtl.connect %res, %a : !firrtl.uint<10>, !firrtl.uint<10>
}
}
