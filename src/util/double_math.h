//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cmath>

/**
 * \brief Math functions for calculations with doubles.
 */
namespace eda::util::doubleMath {

static constexpr auto EPSDOUBLE = std::numeric_limits<double>::epsilon();

/// @brief compares doubles with specified prescision
/// @param a first value
/// @param b second value
/// @param prescision fabs(a-b) < prescision, defaults to machine double epsilon
/// @return true if values are equal with given prescision
inline bool eqvDouble(double a, double b, double prescision = EPSDOUBLE) {
    return std::fabs(a - b) < prescision;
}
/// @brief Computs value in point xt
/// by linear interpolation between points x1 and x2
/// @param val1 value in x1
/// @param val2 value in x2
/// @param x1 coordinate value of x1
/// @param x2 coordinate value of x2
/// @param xt target coordinate for computed value
/// @return value in point xt
inline double linearInterpolation(double val1, double val2,
                                  double x1, double x2, double xt) {
  double r10 = (x2 - xt) / (x2 - x1) * val1;
  double r11 = (xt - x1) / (x2 - x1) * val2;
  double res = r10 + r11;
  return res;
}

// Bilinear interpolation
//    │
//    │Q12|     .R2    |Q22
// y2 │---┼------------┼----
//    │   |     .      |
//    │   |     .      |
// yt │...|............|...
//    │   |     .      |
//    │   |     .      |
//    │   |     .      |
// y1 │---┼------------┼----
//    │Q11|     .R1    |Q21
//    └─────────────────────
//       x1     xt     x2

/// @brief computes value in point (xt, yt) by performing two linear
/// interpolations on X axis and one on Y axis
/// @param val11 value in point (x1,y1)
/// @param val12 value in point (x1,y2)
/// @param val21 value in point (x2,y1)
/// @param val22 value in point (x2,y2)
/// @param x1 coordinate value of x1
/// @param x2 coordinate value of x2
/// @param xt target X coordinate for computed value
/// @param y1 coordinate value of y1
/// @param y2 coordinate value of y2
/// @param yt target Y coordinate for computed value
/// @return value in point (xt, yt)
inline double bilinearInterpolation(double val11, double val12,
                                    double val21, double val22,
                                    double x1, double x2, double xt,
                                    double y1, double y2, double yt) {
#if 1 //optimized coefficient calculations
  const double coef1 = (x2 - xt) / (x2 - x1);
  const double coef2 = (xt - x1) / (x2 - x1);
  double r1 = coef1 * val11 + coef2 * val21;
  double r2 = coef1 * val12 + coef2 * val22;
#else
  double r1 = linearInterpolation(val11, val21, x1, x2, xt);
  double r2 = linearInterpolation(val12, val22, x1, x2, xt);
#endif
  double p = linearInterpolation(r1, r2, y1, y2, yt);
  return p;
}

} // namespace eda::util::doubleMath