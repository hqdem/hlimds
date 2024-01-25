firrtl.circuit "SimpleConstant" {
firrtl.module @SimpleConstant(out %res : !firrtl.sint<4>) {
  %a = firrtl.constant 7 : !firrtl.sint<4>
  firrtl.connect %res, %a : !firrtl.sint<4>, !firrtl.sint<4>
}
}
