module multiplexer(Y, X0, X1, A);

  input X0, X1, A;
  output Y;

  wire Not1Y, And1Y, And2Y;

  not Not1(Not1Y, A);
  and And1(And1Y, X0, Not1Y);
  and And2(And2Y, X1, A);
  or Or1(Y, And2Y, And1Y);

endmodule
