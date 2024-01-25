FIRRTL version 1.1.0
circuit MyModule :     ; CHECK: firrtl.circuit "MyModule" {

  ; CHECK-LABEL: firrtl.module @MyModule(in %in: !firrtl.uint<8>, out %out: !firrtl.uint<8>) {
  module MyModule :   @[FooBar.scala 369:27]
    input in: UInt<8>
    output out: UInt<8>

    ; CHECK: firrtl.connect %out, %in : !firrtl.uint<8>, !firrtl.uint<8>
    out <= in

  ; CHECK: }
