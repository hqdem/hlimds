//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

/**
 * \brief Utility methods for arithmetic-related tasks.
 */
namespace eda::utils {

//===--------------------------------------------------------------------===//
// Num-to-string conversion
//===--------------------------------------------------------------------===//

/** 
 * \brief Generates binary representation of the specified size for a number.
 *
 * Sample output: 
 *
 * to2(6, 4) = "1100", to2(4, 4) = "0100", to2(6, 3) = "110"
 *
 * @return binary representation for a number
 */
std::string toBinString(int num, uint64_t size);

/** 
* Generates the vector, each element is a position of "1" in argument.
*
* Sample output: 
*
* popcnt(6) = {1,2} as 6 in binary is "110"
*
* @return vector of positions of ones in the binary representation of the number
*/
std::vector<int> popcnt(int num);

} // namespace eda::utils

