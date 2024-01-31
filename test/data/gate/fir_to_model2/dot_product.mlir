firrtl.circuit "DotProduct" {
firrtl.module @DotProduct(in %a0 : !firrtl.uint<10>, in %b0: !firrtl.uint<10>, in %a1 : !firrtl.uint<10>, in %b1 : !firrtl.uint<10>, out %res : !firrtl.uint<21>) {
  %0 = firrtl.mul %a0, %b0 : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<20>
  %1 = firrtl.mul %a1, %b1 : (!firrtl.uint<10>, !firrtl.uint<10>) -> !firrtl.uint<20>
  %2 = firrtl.add %0, %1 : (!firrtl.uint<20>, !firrtl.uint<20>) -> !firrtl.uint<21>
  firrtl.connect %res, %2 : !firrtl.uint<21>, !firrtl.uint<21>
}
}
