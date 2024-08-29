//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "util/kitty_utils.h"

#include "kitty/kitty.hpp"
#include "gtest/gtest.h"

namespace eda::util {
  
TEST(KittyUtilsTest, FindRepeatLiteral) {
  SOP sop1 {
    Cube{0b0010, 0b1010},
    Cube{0b1100, 0b1110},
    Cube{0b1001, 0b1001}
  };
  Cube lit1 = Cube{0b1000, 0b1000};
  Cube lit1Copy = findAnyRepeatLiteral(sop1); 
  EXPECT_EQ(lit1, lit1Copy);
  
  SOP sop2 {
      Cube{0b0001, 0b1011},
      Cube{0b0100, 0b1101},
      Cube{0b0111, 0b1111}
  };
  Cube lit2 = Cube{0b0000, 0b1000};
  Cube lit2Copy = findAnyRepeatLiteral(sop2); 
  EXPECT_EQ(lit2, lit2Copy);

  SOP sop3 {
      Cube{0b1000, 0b1010},
      Cube{0b0010, 0b0010},
      Cube{0b0000, 0b1000}
  };
  Cube lit3 = Cube{0b0000, 0b0000};
  Cube lit3Copy = findAnyRepeatLiteral(sop3); 
  EXPECT_EQ(lit3, lit3Copy);
}

TEST(KittyUtilsTest, FindAnyLevel0Kernel) {
  SOP func1 {
    Cube{0b00011101000,0b00011101000},
    Cube{0b00011011000,0b00011011000},
    Cube{0b00001000100,0b00001000100},
    Cube{0b00100000100,0b00100000100},
    Cube{0b00000100011,0b00000100011},
    Cube{0b00000010011,0b00000010011},
    Cube{0b00011110000,0b00011110000}
  };

  SOP kernel1 {
    Cube{0b0000100000,0b0000100000},
    Cube{0b0000010000,0b0000010000}
  };

  Cube lit1(0b00000001000,0b00000001000);
  Cube lit1Copy = findAnyRepeatLiteral(func1);
  EXPECT_EQ(lit1, lit1Copy);

  SOP quotient1 {
    Cube{0b00011100000,0b00011100000},
    Cube{0b00011010000,0b00011010000},
  };
  SOP quotient1Copy = findDivideByLiteralQuotient(func1, lit1);
  EXPECT_EQ(quotient1, quotient1Copy);

  EXPECT_EQ(kernel1, findAnyLevel0Kernel(func1));

  SOP func2 {
    Cube{0b001110,0b001110},
    Cube{0b001011,0b001011},
    Cube{0b001101,0b001101},
    Cube{0b011000,0b011000},
    Cube{0b101010,0b101010}
  };

  SOP kernel2 {
    Cube{0b000100,0b000100},
    Cube{0b000001,0b000001},
    Cube{0b100000,0b100000}
  };

  Cube lit2(0b000010,0b000010);
  Cube lit2Copy = findAnyRepeatLiteral(func2);
  EXPECT_EQ(lit2, lit2Copy);

  SOP quotient2 {
    Cube{0b001100,0b001100},
    Cube{0b001001,0b001001},
    Cube{0b101000,0b101000}
  };

  SOP quotient2Copy = findDivideByLiteralQuotient(func2, lit2);
  EXPECT_EQ(quotient2, quotient2Copy);

  EXPECT_EQ(kernel2, findAnyLevel0Kernel(func2));
}

} // namespace eda::util
