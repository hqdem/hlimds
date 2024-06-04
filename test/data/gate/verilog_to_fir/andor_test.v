module andor(clk, a, b, c);
  input  wire clk;
  input  wire [3:0] a;
  input  wire [3:0] b;
  output reg [3:0] c;

  assign c = a & b | b;

endmodule