firrtl.circuit "OutTo" {
firrtl.module @OutTo(in %a : !firrtl.uint<10>, out %res0 : !firrtl.uint<10>, out %res1 : !firrtl.uint<10>) {
  firrtl.connect %res0, %a : !firrtl.uint<10>, !firrtl.uint<10>
  firrtl.connect %res1, %res0 : !firrtl.uint<10>, !firrtl.uint<10>
}
}
