#include <percy/percy.hpp>

#include <cassert>

int main() { 
  percy::spec s;
  s.set_nr_out(1);

  percy::chain c;

  kitty::dynamic_truth_table func(4);
  kitty::create_from_hex_string(func, "0117");

  s[0] = func;

  auto const result = synthesize( s, c );
  assert(result == percy::success);

  c.print_expression();

  return 0;
}
