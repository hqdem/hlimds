#include "delay_estmt.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace eda::gate::tech_optimizer::delay_estimation {

using std::cout;
using std::filesystem::path;
using std::getline;
using std::ifstream;
using std::max_element;
using std::min_element;
using std::stof;
using std::string;
using std::vector;

WLM::WLM() :
  wire_load_name("top"),
  R(fudge * 0.00008), C(fudge * 0.000030),
  Area(1), slope(length_top * 0.5) {
  /* Resistance is 0.03ff/micron for avg metal         */
  /* Capacitance is 80 m-ohm/square, in kohm units     */
  /* (remember that our capacitance unit is 1.0pf)     */

  set_wire_load_model(wire_load_name);
}

WLM::WLM(string name) :
  R(fudge * 0.00008), C(fudge * 0.000030),
  Area(1), slope(length_top * 0.5) {

  set_wire_load_model(name);
}

void WLM::set_wire_load_model(std::string &wlm_name) {
  if ((wire_load_name == "top") || (wire_load_name == "10k") ||
      (wire_load_name == "5k")  || (wire_load_name == "2k")  ||
      (wire_load_name == "1k")  || (wire_load_name == "500")) {
    
    wire_load_name = wlm_name;
  
    float length;
    float mp = 1;
    
    if (wire_load_name == "top")
      length = length_top;
    else if (wire_load_name == "10k")
      length = length_10k;
    /* sqrt(5000/10000) = .71  *
    * sqrt(2000/10000) = .45   *
    * sqrt(1000/10000) = .32   *
    * sqrt(500/10000) = .22    */
    else if (wire_load_name == "5k")
      length = length_10k * 0.71;
    else if (wire_load_name == "2k")
      length = length_10k * 0.45;
    else if (wire_load_name == "1k")
      length = length_10k * 0.32;
    else if (wire_load_name == "500")
      length = length_10k * 0.22;
    for (int i = 0; i < 8; ++i) {
        fanout_length[i] = std::make_pair(i+1,length*mp);
        mp += 0.5;
      }
    for (int i = 0; i < 6; ++i) 
      fanout_resistance[i] = std::make_pair(i+1, fanout_length[i].second*R);
    for (int i = 0; i < 6; ++i)
      fanout_capacitance[i] = std::make_pair(i+1, fanout_length[i].second*C);
  }

  else
    std::cerr << "WLM: wrong name\n";
}


/// Getters
  float NLDM::getCellDelay()   {
    return delay;
  }

  float NLDM::getSlew()   {
    return slew;
  }

  float WLM::getFanoutCap(int& fanout_count) {
    if ((fanout_count > 0) && (fanout_count < 8)) 
      return fanout_capacitance[fanout_count-1].second;
    else
      return 0;
    /// TODO: make an extrapolation
  }

  
///Inter-/Extra-polation
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

  void NLDM::delayEstimation(string& cell_name,
                       const char* file_name,
                       float& input_net_transition,
                       float& total_output_net_capacitance )   {
    /*const path homePath = string(getenv("UTOPIA_HOME"));
    const path filePath = homePath / "src" / "gate" 
                        / "techoptimizer" 
                        / "cut_based_tech_mapper" 
                        / "strategy" 
                        / "delay_estmt"
                        / "attrs.txt";
    string file = filePath.generic_string();
    ifstream f(file);*/
    //===----------------------------------------------------------------------===//
    //  Properties
    //===----------------------------------------------------------------------===//
      size_t ind_1 = -1, ind_2 = -1;
      std::vector<float> temp = {};
      std::vector<float> cfall = {}, crise = {}, tfall = {}, trise = {};
      bool ivar = false;
      capacitance = 0;

      float x1, x2, y1, y2, T11, T12, T21, T22, T00;
      int tback1 = 0, tfront1 = 0;
      int tback2 = 0, tfront2 = 0;
    //===----------------------------------------------------------------------===//
    //  Call for parser
    //===----------------------------------------------------------------------===//
      TokenParser tokParser;
      FILE *file = fopen(file_name, "rb");
      Group *ast = tokParser.parseLibrary(file, file_name);
      Library lib;
      AstParser parser(lib, tokParser);
      parser.run(*ast);
      fclose(file);
      
    //===----------------------------------------------------------------------===//
    //  Delay and Slew estimation
    //===----------------------------------------------------------------------===//

      const Cell *cell = lib.getCell(cell_name);

      for (const Pin &pin : (*cell).getPins()) {
        capacitance += pin.getFloatAttribute("capacitance", 0);
        for (const Timing &timing : pin.getTimings()) {
          auto *lutP1 = timing.getLut("cell_fall");
          auto *lutP2 = timing.getLut("cell_rise");
          auto *lutP3 = timing.getLut("fall_transition");
          auto *lutP4 = timing.getLut("rise_transition");

        /// CELL FALL
          if (lutP1 != nullptr) {
            temp = lutP1->getValues();
            ind_1 = -1, ind_2 = -1;
            ivar = false;
            for (const auto &it : (*lutP1)) {
              if (!ivar && (std::find(it.values.begin(), it.values.end(), input_net_transition) == std::end(it.values))) {
                break;
              }
              else if (std::find(it.values.begin(), it.values.end(), input_net_transition) != std::end(it.values)) {
                if (!ivar) {
                  ivar = true;
                  for (int i = 0; i < it.values.size(); ++i) {
                    if (it.values[i] == input_net_transition) {
                      ind_1 = i;
                      break;
                    }
                  }
                }
              }
              else if (ivar && (std::find(it.values.begin(), it.values.end(), total_output_net_capacitance) != std::end(it.values))) {
                for (int i = 0; i < it.values.size(); ++i) {
                  if (it.values[i] == total_output_net_capacitance) {
                    ind_2 = i;
                    cfall.push_back(temp[ind_1 * it.values.size() + ind_2]);
                    break;
                  }
                } 
              }
            }

            /// INTERPOLATION
            if ((ind_1 == -1) && (ind_2 == -1)) {
              T11 = 0, T12 = 0, T21 = 0, T22 = 0;
              tback1 = 0, tfront1 = 0;
              tback2 = 0, tfront2 = 0;
              for (const auto &it : (*lutP1)) {
                if (!ivar) {
                  for (int i = 0; i < it.values.size(); ++i) {
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
                else if (ivar) {
                  for (int i = 0; i < it.values.size(); ++i) {
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
                  T11 = temp[tback1 * it.values.size() + tback2];
                  T12 = temp[tback1 * it.values.size() + tfront2];
                  T21 = temp[tfront1 * it.values.size() + tback2];
                  T22 = temp[tfront1 * it.values.size() + tfront2];
                  T00 = interpolation(input_net_transition, total_output_net_capacitance, 
                                      x1, x2, y1, y2, T11, T12, T21, T22);
                  cfall.push_back(T00);
                }
              }
            }
          }
        /// CELL RISE
          if (lutP2 != nullptr) {
            temp = lutP2->getValues();
            ind_1 = -1, ind_2 = -1;
            ivar = false;
            for (const auto &it : (*lutP2)) {
              if (!ivar && (std::find(it.values.begin(), it.values.end(), input_net_transition) == std::end(it.values))) {
                break;
              }
              else if (std::find(it.values.begin(), it.values.end(), input_net_transition) != std::end(it.values)) {
                if (!ivar) {
                  ivar = true;
                  for (int i = 0; i < it.values.size(); ++i) {
                    if (it.values[i] == input_net_transition) {
                      ind_1 = i;
                      break;
                    }
                  }
                }
              }
              else if (ivar && (std::find(it.values.begin(), it.values.end(), total_output_net_capacitance) != std::end(it.values))) {
                for (int i = 0; i < it.values.size(); ++i) {
                  if (it.values[i] == total_output_net_capacitance) {
                    ind_2 = i;
                    crise.push_back(temp[ind_1 * it.values.size() + ind_2]);
                    break;
                  }
                } 
              }
            }

            /// INTERPOLATION
            if ((ind_1 == -1) && (ind_2 == -1)) {
              T11 = 0, T12 = 0, T21 = 0, T22 = 0;
              tback1 = 0, tfront1 = 0;
              tback2 = 0, tfront2 = 0;
              for (const auto &it : (*lutP2)) {
              if (!ivar) {
                  for (int i = 0; i < it.values.size(); ++i) {
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
                else if (ivar) {
                  for (int i = 0; i < it.values.size(); ++i) {
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
                  T11 = temp[tback1 * it.values.size() + tback2];
                  T12 = temp[tback1 * it.values.size() + tfront2];
                  T21 = temp[tfront1 * it.values.size() + tback2];
                  T22 = temp[tfront1 * it.values.size() + tfront2];
                  T00 = interpolation(input_net_transition, total_output_net_capacitance, 
                                      x1, x2, y1, y2, T11, T12, T21, T22);
                  crise.push_back(T00);
                }
              }
            }
          }
          /// FALL TRANSITION
          if (lutP3 != nullptr) {
            temp = lutP3->getValues();
            ind_1 = -1, ind_2 = -1;
            ivar = false;
            for (const auto &it : (*lutP3)) {
              if (!ivar && (std::find(it.values.begin(), it.values.end(), input_net_transition) == std::end(it.values))) {
                break;
              }
              else if (std::find(it.values.begin(), it.values.end(), input_net_transition) != std::end(it.values)) {
                if (!ivar) {
                  ivar = true;
                  for (int i = 0; i < it.values.size(); ++i) {
                    if (it.values[i] == input_net_transition) {
                      ind_1 = i;
                      break;
                    }
                  }
                }
              }
              else if (ivar && (std::find(it.values.begin(), it.values.end(), total_output_net_capacitance) != std::end(it.values))) {
                for (int i = 0; i < it.values.size(); ++i) {
                  if (it.values[i] == total_output_net_capacitance) {
                    ind_2 = i;
                    tfall.push_back(temp[ind_1 * it.values.size() + ind_2]);
                    break;
                  }
                } 
              }
            }

            /// INTERPOLATION
            if ((ind_1 == -1) && (ind_2 == -1)) {
              T11 = 0, T12 = 0, T21 = 0, T22 = 0;
              tback1 = 0, tfront1 = 0;
              tback2 = 0, tfront2 = 0;
              for (const auto &it : (*lutP3)) {
                if (!ivar) {
                  for (int i = 0; i < it.values.size(); ++i) {
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
                else if (ivar) {
                  for (int i = 0; i < it.values.size(); ++i) {
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
                  T11 = temp[tback1 * it.values.size() + tback2];
                  T12 = temp[tback1 * it.values.size() + tfront2];
                  T21 = temp[tfront1 * it.values.size() + tback2];
                  T22 = temp[tfront1 * it.values.size() + tfront2];
                  T00 = interpolation(input_net_transition, total_output_net_capacitance, 
                                      x1, x2, y1, y2, T11, T12, T21, T22);
                  tfall.push_back(T00);
                }
              }
            }
          }
          /// RISE TRANSITION
          if (lutP4 != nullptr) {
            temp = lutP4->getValues();
            ivar = false;
            ind_1 = -1, ind_2 = -1;
            for (const auto &it : (*lutP4)) {
              if (!ivar && (std::find(it.values.begin(), it.values.end(), input_net_transition) == std::end(it.values))) {
                break;
              }
              else if (std::find(it.values.begin(), it.values.end(), input_net_transition) != std::end(it.values)) {
                if (!ivar) {
                  ivar = true;
                  for (int i = 0; i < it.values.size(); ++i) {
                    if (it.values[i] == input_net_transition) {
                      ind_1 = i;
                      break;
                    }
                  }
                }
              }
              else if (ivar && (std::find(it.values.begin(), it.values.end(), total_output_net_capacitance) != std::end(it.values))) {
                for (int i = 0; i < it.values.size(); ++i) {
                  if (it.values[i] == total_output_net_capacitance) {
                    ind_2 = i;
                    trise.push_back(temp[ind_1 * it.values.size() + ind_2]);
                    break;
                  }
                } 
              }
            }

            /// INTERPOLATION
            if ((ind_1 == -1) && (ind_2 == -1)) {
              T11 = 0, T12 = 0, T21 = 0, T22 = 0;
              tback1 = 0, tfront1 = 0;
              tback2 = 0, tfront2 = 0;
              for (const auto &it : (*lutP4)) {
                if (!ivar) {
                  for (int i = 0; i < it.values.size(); ++i) {
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
                else if (ivar) {
                  for (int i = 0; i < it.values.size(); ++i) {
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
                  T11 = temp[tback1 * it.values.size() + tback2];
                  T12 = temp[tback1 * it.values.size() + tfront2];
                  T21 = temp[tfront1 * it.values.size() + tback2];
                  T22 = temp[tfront1 * it.values.size() + tfront2];
                  T00 = interpolation(input_net_transition, total_output_net_capacitance, 
                                      x1, x2, y1, y2, T11, T12, T21, T22);
                  trise.push_back(T00);
                }
              }
            }
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

} // namespace eda::gate::tech_optimizer::delay_estimation
