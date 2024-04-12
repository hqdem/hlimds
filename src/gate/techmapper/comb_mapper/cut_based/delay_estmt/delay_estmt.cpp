//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/comb_mapper/cut_based/delay_estmt/delay_estmt.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace eda::gate::techmapper {

using std::filesystem::exists;
using std::filesystem::path;
using std::getline;
using std::ifstream;
using std::max_element;
using std::min_element;
using std::stof;
using std::string;
using std::vector;

WLM::WLM() :
  wire_load_name("sky"),
  r(fudge * 0.08), c(fudge * 0.0002),
  slope(8.3631) {
  /* Capacitance is 0.02ff/micron for avg metal         */
  /* Resistance is 80 m-ohm/square, in kohm units     */
  /* (remember that our capacitance unit is 1.0pf)     */

  setWireLoadModel(wire_load_name);
}

WLM::WLM(string name) :
  r(fudge * 0.004), c(fudge * 0.2),
  slope(6.2836) {

  setWireLoadModel(name);
}

void WLM::setWireLoadModel(string wlm_name) {
  if ((wlm_name == "sky") || (wlm_name == "5k") ||
      (wlm_name == "3k")  || (wlm_name == "1k") ) {

    wire_load_name = wlm_name;
    float length;
    float mp_sky[] = {1.0, 1.38, 2.08, 2.75, 3.71, 3.62};
    float mp_5k[] = {1.0, 2.1, 3.53, 5.51, 8.31, 11.70};
    float mp_3k[] = {1.0, 2.49, 3.20, 6.19, 8.59, 11.50};
    float mp_1k[] = {1.0, 2.26, 3.70, 5.28, 6.82, 8.50};
    float multip[6];

    for (size_t i = 0; i < 6; ++i)
      fanout_length[i].first = i+1;

    if (wire_load_name == "sky") {
      length = length_sky;
      for (size_t i = 0; i < 6; ++i)
        multip[i] = mp_sky[i];

      /// Changing properties
      r = fudge * 0.08;
      c = fudge * 0.00002;
      slope = 8.3631;
    }
    else if (wire_load_name == "5k") {
      length = length_5k;
      for (size_t i = 0; i < 6; ++i)
        multip[i] = mp_5k[i];

      /// Changing properties
      r = fudge * 0.004;
      c = fudge * 0.2;
      slope = 6.2836;
    }
    else if (wire_load_name == "3k") {
      length = length_3k;
      for (size_t i = 0; i < 6; ++i)
        multip[i] = mp_3k[i];
      
      /// Changing properties
      r = fudge * 0.004;
      c = fudge * 0.2;
      slope = 6.2836;
    }
    else if (wire_load_name == "1k") {
      length = length_1k;
      for (size_t i = 0; i < 6; ++i)
        multip[i] = mp_1k[i];

      /// Changing properties
      r = fudge * 0.004;
      c = fudge * 0.2;
      slope = 6.2836;
    }

    for (size_t i = 0; i < 6; ++i)
      fanout_length[i].second = length*multip[i];

    for (size_t i = 0; i < 6; ++i)
      fanout_resistance[i] = std::make_pair(i+1, fanout_length[i].second * r);

    for (size_t i = 0; i < 6; ++i)
      fanout_capacitance[i] = std::make_pair(i+1, fanout_length[i].second * c);
  }

  else
    std::cerr << "WLM: wrong name\n";
}

/// Getters
float WLM::getLength(size_t& fanout_count) {
  if ((fanout_count > 0) && (fanout_count < 6))
    return fanout_length[fanout_count-1].second;
  else if (fanout_count > 6)
    return fanout_length[5].second + (fanout_count - 6) * slope;

  std::cerr << "Wrong fanout_count\n";
  return 0;
}

float WLM::getFanoutCap(size_t& fanout_count) {
  if ((fanout_count > 0) && (fanout_count < 6))
    return fanout_capacitance[fanout_count-1].second;
  else if (fanout_count > 6) {
    float length = fanout_length[5].second + (fanout_count - 6) * slope;
    return length * c;
  }
  
  std::cerr << "Wrong fanout_count\n";
  return 0;
}

float WLM::getFanoutRes(size_t& fanout_count) {
  if ((fanout_count > 0) && (fanout_count < 6))
    return fanout_resistance[fanout_count-1].second;
  else if (fanout_count > 6) {
    float length = fanout_length[5].second + (fanout_count - 6) * slope;
    return length * r;
  }
  std::cerr << "Wrong fanout_count\n";
  return 0;
}

/// Inter-/Extra-polation
float interpolation(float x0, float y0,
                    float x1, float x2,
                    float y1, float y2,
                    float T11, float T12,
                    float T21, float T22) {

  float x01 = 0, x20 = 0, y01 = 0, y20 = 0;

  x01 = (x0 - x1) / (x2 - x1);
  x20 = (x2 - x0) / (x2 - x1);
  y01 = (y0 - y1) / (y2 - y1);
  y20 = (y2 - y0) / (y2 - y1);

  float T00 = x20*y20*T11 + x20*y01*T12 + x01*y20*T21 + x01*y01*T22;

  return T00;
}

float NLDM::lutInterpolation(const LookupTable *lut, size_t variablesCount, 
                       float& input_net_transition,
                       float& total_output_net_capacitance,
                       float& x1, float& x2, float& y1, float& y2, 
                       size_t& back1, size_t& front1, size_t& back2, size_t& front2) {
  float T00 = 0, T11 = 0, T12 = 0, T21 = 0, T22 = 0;
  vector<float> lut_values = lut->getValues();

  T11 = lut_values[back1 * variablesCount + back2];
  T12 = lut_values[back1 * variablesCount + front2];
  T21 = lut_values[front1 * variablesCount + back2];
  T22 = lut_values[front1 * variablesCount + front2];

  T00 = interpolation(input_net_transition,
                      total_output_net_capacitance,
                      x1, x2, y1, y2, T11, T12, T21, T22);

  return T00;
}

float NLDM::timingVisitor(const Timing &timing,
        string dtype,
        float& input_net_transition,
        float& total_output_net_capacitance) {

  const auto *lut = timing.getLut(dtype);

  if (lut != nullptr) {
    //===-----------------------------------------------------------------===//
    //  Properties
    //===-----------------------------------------------------------------===//
    bool ivar = false;
    int ind_1 = -1, ind_2 = -1;
    vector<float> lut_values = {};
    float x1, x2, y1, y2, T00, T11 = 0, T12 = 0, T21 = 0, T22 = 0;
    size_t tback1 = 0, tfront1 = 0;
    size_t tback2 = 0, tfront2 = 0;

    lut_values = lut->getValues();

    //===-----------------------------------------------------------------===//
    //  Assigning values from LookUp-Tables
    //===-----------------------------------------------------------------===//

    /// FOUND VALUES
    for (const auto &it : (*lut)) {
      if (!ivar && (std::find(it.values.begin(), it.values.end(),
          input_net_transition) == std::end(it.values))) {
        break;
      }
      else if (!ivar && (std::find(it.values.begin(), it.values.end(),
                         input_net_transition) != std::end(it.values))) {
        ivar = true;
        for (size_t i = 0; i < it.values.size(); ++i) {
          if (it.values[i] == input_net_transition) {
            ind_1 = i;
            break;
          }
        }
      }
      else if (ivar && (std::find(it.values.begin(), it.values.end(),
               total_output_net_capacitance) != std::end(it.values))) {
        for (size_t i = 0; i < it.values.size(); ++i) {
          if (it.values[i] == total_output_net_capacitance) {
            ind_2 = i;
            return lut_values[ind_1 * it.values.size() + ind_2];
          }
        }
      }
    }

    /// INTERPOLATION
    if ((ind_1 == -1) && (ind_2 == -1)) {
      tback1 = 0, tfront1 = 0;
      tback2 = 0, tfront2 = 0;
      x1 = 0, x2 = 0, y1 = 0, y2 = 0;
      for (const auto &it : (*lut)) {
        if (!ivar) {
          for (size_t i = 0; i < it.values.size(); ++i) {
            if (it.values[i] < input_net_transition) {
              tback1 = i;
              x1 = it.values[i];
            }
            else if (it.values[i] > input_net_transition) {
              tfront1 = i;
              x2 = it.values[i];
              break;
            }
          }
          ivar = true;
        }
        else {
          for (size_t i = 0; i < it.values.size(); ++i) {
            if (it.values[i] < total_output_net_capacitance) {
              tback2 = i;
              y1 = it.values[i];
            }
            else if (it.values[i] > total_output_net_capacitance) {
              tfront2 = i;
              y2 = it.values[i];
              break;
            }
          }
          T11 = lut_values[tback1 * it.values.size() + tback2];
          T12 = lut_values[tback1 * it.values.size() + tfront2];
          T21 = lut_values[tfront1 * it.values.size() + tback2];
          T22 = lut_values[tfront1 * it.values.size() + tfront2];
          T00 = interpolation(input_net_transition,
                              total_output_net_capacitance,
                              x1, x2, y1, y2, T11, T12, T21, T22);
          return T00;
        }
      }
    }
  }
  return -1;
}

vector<float> NLDM::timingVisitor(const Timing &timing,
        float& input_net_transition,
        float& total_output_net_capacitance) {

  //===-----------------------------------------------------------------===//
  //  LookUp Tables
  //===-----------------------------------------------------------------===//
  const auto *lutcf = timing.getLut("cell_fall");
  const auto *lutcr = timing.getLut("cell_rise");
  const auto *luttf = timing.getLut("fall_transition");
  const auto *luttr = timing.getLut("rise_transition");
  //===-----------------------------------------------------------------===//
  //  Properties
  //===-----------------------------------------------------------------===//
  int ind_1 = -1, ind_2 = -1;
  vector<float> lut_values = {}, result = {};
  size_t variablesCount = 0;
  /// For interpolation
  float x1 = 0, x2 = 0, y1 = 0, y2 = 0,
        T00 = 0, T11 = 0, T12 = 0, T21 = 0, T22 = 0;
  size_t tback1 = 0, tfront1 = 0;
  size_t tback2 = 0, tfront2 = 0;

  if (lutcf != nullptr) {
    //===-----------------------------------------------------------------===//
    //  Properties
    //===-----------------------------------------------------------------===//
    bool ivar = false;
    lut_values = lutcf->getValues();

    //===-----------------------------------------------------------------===//
    //  Assigning values from LookUp Tables
    //===-----------------------------------------------------------------===//
    /// FOUND VALUES
    for (const auto &it : (*lutcf)) {
      if (!ivar && (std::find(it.values.begin(), it.values.end(),
          input_net_transition) == std::end(it.values))) {
        break;
      }
      else if (!ivar && (std::find(it.values.begin(), it.values.end(),
                         input_net_transition) != std::end(it.values))) {
        ivar = true;
        for (size_t i = 0; i < it.values.size(); ++i) {
          if (it.values[i] == input_net_transition) {
            ind_1 = i;
            break;
          }
        }
      }
      else if (ivar && (std::find(it.values.begin(), it.values.end(),
               total_output_net_capacitance) != std::end(it.values))) {
        for (size_t i = 0; i < it.values.size(); ++i) {
          if (it.values[i] == total_output_net_capacitance) {
            ind_2 = i;
            variablesCount = it.values.size();
            result.push_back(lut_values[ind_1 * variablesCount + ind_2]);
          }
        }
      }
    }

    /// INTERPOLATION
    if ((ind_1 == -1) && (ind_2 == -1)) {
      for (const auto &it : (*lutcf)) {
        if (!ivar) {
          for (size_t i = 0; i < it.values.size(); ++i) {
            if (it.values[i] < input_net_transition) {
              tback1 = i;
              x1 = it.values[i];
            }
            else if (it.values[i] > input_net_transition) {
              tfront1 = i;
              x2 = it.values[i];
              break;
            }
          }
          ivar = true;
        }
        else {
          for (size_t i = 0; i < it.values.size(); ++i) {
            if (it.values[i] < total_output_net_capacitance) {
              tback2 = i;
              y1 = it.values[i];
            }
            else if (it.values[i] > total_output_net_capacitance) {
              tfront2 = i;
              y2 = it.values[i];
              break;
            }
          }
          T11 = lut_values[tback1 * it.values.size() + tback2];
          T12 = lut_values[tback1 * it.values.size() + tfront2];
          T21 = lut_values[tfront1 * it.values.size() + tback2];
          T22 = lut_values[tfront1 * it.values.size() + tfront2];
          T00 = interpolation(input_net_transition,
                              total_output_net_capacitance,
                              x1, x2, y1, y2, T11, T12, T21, T22);
          result.push_back(T00);
        }
      }
    }

    if (lutcr != nullptr) {
      lut_values = lutcr->getValues();
      if ((ind_1 > -1) && (ind_2 > -1)) {
        result.push_back(lut_values[ind_1 * variablesCount + ind_2]);
      }
      else {
        T00 = lutInterpolation(lutcr, variablesCount,
                              input_net_transition,
                              total_output_net_capacitance,
                              x1, x2, y1, y2, 
                              tback1, tfront1, tback2, tfront2);
        result.push_back(T00);
      }

      if (luttf != nullptr) {
        lut_values = luttf->getValues();
        if ((ind_1 > -1) && (ind_2 > -1)) {
          result.push_back(lut_values[ind_1 * variablesCount + ind_2]);
        }
        else {
          T00 = lutInterpolation(luttf, variablesCount,
                                input_net_transition,
                                total_output_net_capacitance,
                                x1, x2, y1, y2, 
                                tback1, tfront1, tback2, tfront2);
          result.push_back(T00);
        }
        
        if (luttr != nullptr) {
          lut_values = luttr->getValues();
          if ((ind_1 > -1) && (ind_2 > -1)) {
            result.push_back(lut_values[ind_1 * variablesCount + ind_2]);
            return result;
          }
          else {
            T00 = lutInterpolation(luttr, variablesCount,
                                  input_net_transition,
                                  total_output_net_capacitance,
                                  x1, x2, y1, y2, 
                                  tback1, tfront1, tback2, tfront2);
            result.push_back(T00);
            
            return result;
          }
        }
      }
    }
  }
  return result;
}


void NLDM::delayEstimation(string& cell_name,
                           string& file_name,
                           float& input_net_transition,
                           float& total_output_net_capacitance) {
  //===-------------------------------------------------------------------===//
  //  Connecting paths
  //===-------------------------------------------------------------------===//
  const path homePath = string(getenv("UTOPIA_HOME"));
  const path filePath = homePath / file_name;
  if (exists(filePath)) {
    //===---------------------------------------------------------------===//
    //  Properties
    //===---------------------------------------------------------------===//
    std::vector<float> cfall = {}, crise = {}, tfall = {}, trise = {};
    capacitance = 0;
    float checker = 0;

    //===---------------------------------------------------------------===//
    //  Call parser
    //===---------------------------------------------------------------===//
    TokenParser tokParser;
    FILE *file = fopen(filePath.generic_string().c_str(), "rb");
    Group *ast = tokParser.parseLibrary(file,
                                        filePath.generic_string().c_str());
    Library lib;
    AstParser parser(lib, tokParser);
    parser.run(*ast);
    fclose(file);

    //===---------------------------------------------------------------===//
    //  Delay and Slew estimation
    //===---------------------------------------------------------------===//
    const Cell *cell = lib.getCell(cell_name);

    for (const Pin &pin : (*cell).getPins()) {
      capacitance += pin.getFloatAttribute("capacitance", 0);
      for (const Timing &timing : pin.getTimings()) {
        checker = timingVisitor(timing,"cell_fall",
          input_net_transition, total_output_net_capacitance);
        if (checker > 0) {
          cfall.push_back(timingVisitor(timing,"cell_fall",
            input_net_transition, total_output_net_capacitance));
          crise.push_back(timingVisitor(timing,"cell_rise",
            input_net_transition, total_output_net_capacitance));
          tfall.push_back(timingVisitor(timing,"fall_transition",
            input_net_transition, total_output_net_capacitance));
          trise.push_back(timingVisitor(timing,"rise_transition",
            input_net_transition, total_output_net_capacitance));
        }
        else {
          std::cerr << "Error occured in NLDM::timingVisitor.\n";
          std::cerr << "Filling values with zeros.\n";
          cfall.push_back(0);
          crise.push_back(0);
          tfall.push_back(0);
          trise.push_back(0);
        }
      }
    }

    delay = *max_element(crise.begin(), crise.end());
    for (size_t i = 0; i < crise.size(); ++i)
      if (crise[i] == delay) {
        slew = (tfall[i] + trise[i])/2;
        break;
      }
  }
  else
    std::cerr << "File wasn't found\n";
}

void NLDM::delayEstimation(string& cell_name,
                           Library& lib,
                           float& input_net_transition,
                           float& total_output_net_capacitance)   {
  //===---------------------------------------------------------------===//
  //  Properties
  //===---------------------------------------------------------------===//
  vector<float> cfall = {}, crise = {}, tfall = {}, trise = {};
  float checker[] = {0, 0 , 0, 0};
  capacitance = 0;

  //===---------------------------------------------------------------===//
  //  Delay and Slew estimation
  //===---------------------------------------------------------------===//
  const Cell *cell = lib.getCell(cell_name);
  
  for (const Pin &pin : (*cell).getPins()) {
    capacitance += pin.getFloatAttribute("capacitance", 0);
    for (const Timing &timing : pin.getTimings()) {
      vector<float> temp = timingVisitor(timing,
          input_net_transition, total_output_net_capacitance);
      for (size_t i = 0; i < 4; ++i) {
        checker[i] = temp[i];
      }
      if (checker[0] > 0) {
        cfall.push_back(checker[0]);
        crise.push_back(checker[1]);
        tfall.push_back(checker[2]);
        trise.push_back(checker[3]);
      }
      else {
        std::cerr << "Error occured in NLDM::timingVisitor.\n";
        std::cerr << "Filling values with zeros.\n";
        cfall.push_back(0);
        crise.push_back(0);
        tfall.push_back(0);
        trise.push_back(0);
      }
    }
  }

  delay = *max_element(crise.begin(), crise.end());
  for (size_t i = 0; i < crise.size(); ++i)
    if (crise[i] == delay) {
      slew = (tfall[i] + trise[i])/2;
      break;
    }
}

} // namespace eda::gate::techmapper
