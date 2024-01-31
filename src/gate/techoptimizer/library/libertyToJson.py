import json
from sys import argv
from liberty.parser import parse_liberty

script, pathToLibraryFile, pathToJsonFile = argv

def read_information_from_library(path_to_library):
  library = parse_liberty(open(path_to_library).read())

  input_pins = ""
  output_pins = {}
  cell_library = {}
  last_output_pins = {}
  delay = {}
  power = {}

  comb_cell = True

  isFF = False
  isFFrs = False
  isLatch = False
  clear = ''
  clear_preset_var1xl = ''
  clear_preset_var2 = ''
  clocked_on= ''
  next_state = ''
  preset = ''
  data_in = ''
  enable = ''

  for cell_group in library.get_groups('cell'):
    isFF = False
    isFFrs = False
    isLatch = False
    clear = ''
    clear_preset_var1xl = ''
    clear_preset_var2 = ''
    clocked_on= ''
    next_state = ''
    preset = ''
    data_in = ''
    enable = ''
    cell_name = (str(cell_group.args[0]))[1: -1]

    worstLeakege = 0
    for leakege_grop in cell_group.get_groups('leakage_power'):
      curLeakege = leakege_grop['value']
      if curLeakege > worstLeakege:
        worstLeakege = curLeakege
    for ff_grup in cell_group.get_groups('ff'):
      clear = str(ff_grup['clear'])[1: -1]
      clear_preset_var1xl = str(ff_grup['clear_preset_var1'])[1: -1]
      clear_preset_var2 = str(ff_grup['clear_preset_var2'])[1: -1]
      clocked_on = str(ff_grup['clocked_on'])[1: -1]
      next_state = str(ff_grup['next_state'])[1: -1]
      preset = str(ff_grup['preset'])[1: -1]

      if (clocked_on != "" and next_state != "" and clear == ""):
        isFF = True

      elif (clocked_on != "" and next_state != "" and clear != ""):
        isFFrs = True

    for latch_grup in cell_group.get_groups('latch'):
        clear = str(latch_grup['clear'])[1: -1]
        data_in = str(latch_grup['data_in'])[1: -1]
        enable = str(latch_grup['enable'])[1: -1]
        if (data_in != ""):
            isLatch = True

    for pin_group in cell_group.get_groups('pin'):
      pin_name_attribute = str(pin_group.args[0])[1: -1]

      cell_area = cell_group['area']

      pin_direction_attribute = pin_group['direction']

      if pin_direction_attribute == "output":
        pin_function_attribute = str(pin_group["function"])[1: -1]
        output_pins[pin_name_attribute] = pin_function_attribute

        for power_group in pin_group.get_groups('internal_power'):
          fall_power_max_value = 0
          rise_power_max_value = 0
          related_pin = str(power_group["related_pin"])[1: -1]

          for fall_power_group in power_group.get_groups('fall_power'):
            fall = fall_power_group.get_array('values')
            max_value = max(max(row) for row in fall)
            if max_value > fall_power_max_value:
              fall_power_max_value = max_value

          for rise_power_group in power_group.get_groups('rise_power'):
            rise = rise_power_group.get_array('values')
            max_value = max(max(row) for row in rise)
            if max_value > rise_power_max_value:
              rise_power_max_value = max_value


          power[related_pin] = {'fall_power': fall_power_max_value,
                                'rise_power': rise_power_max_value
                                }
        for timing_group in pin_group.get_groups('timing'):
          cell_fall_max_value = 0
          cell_rise_max_value = 0
          fall_transition_max_value = 0
          rise_transition_max_value = 0
          related_pin = str(timing_group["related_pin"])[1: -1]

          for cell_fall_group in timing_group.get_groups('cell_fall'):
              cell_fall = cell_fall_group.get_array('values')
              max_value = max(max(row) for row in cell_fall)
              if max_value > cell_fall_max_value:
                cell_fall_max_value = max_value

          for cell_rise_group in timing_group.get_groups('cell_rise'):
            cell_rise = cell_rise_group.get_array('values')
            max_value = max(max(row) for row in cell_rise)
            if max_value > cell_rise_max_value:
              cell_rise_max_value = max_value

          for fall_transition_group in timing_group.get_groups('fall_transition'):
            fall_transition = fall_transition_group.get_array('values')
            max_value = max(max(row) for row in fall_transition)
            if max_value > fall_transition_max_value:
              fall_transition_max_value = max_value

          for rise_transition_group in timing_group.get_groups('rise_transition'):
            rise_transition = rise_transition_group.get_array('values')
            max_value = max(max(row) for row in rise_transition)
            if max_value > rise_transition_max_value:
              rise_transition_max_value = max_value
          delay[related_pin] = {'cell_fall': cell_fall_max_value,
                                'cell_rise': cell_rise_max_value,
                                'fall_transition': fall_transition_max_value,
                                'rise_transition': rise_transition_max_value,
                                'fall_power': fall_power_max_value,
                                'rise_power': rise_power_max_value
                                }

      else:
        input_pins += pin_name_attribute + " "

    if (("Q" not in output_pins)
        and ("CLK" not in input_pins) and (len(output_pins) != 0)):
      cell_library[cell_name] = {'comb': True,
                                 'delay': delay,
                                 'input': input_pins,
                                 'output': output_pins,
                                 'area' : cell_area,
                                 'leakage' : worstLeakege,
                                 'power': power
                                 }
      last_output_pins = output_pins
    elif isFF or isFFrs:
      cell_library[cell_name] = {'comb': False,
                                 'delay': delay,
                                 'input': input_pins,
                                 'output': output_pins,
                                 'area' : cell_area,
                                 'leakage' : worstLeakege,
                                 'power': power,
                                 'ff' : isFF,
                                 'ffrs' : isFFrs,
                                 'latch' : isLatch,
                                 'clear' : clear,
                                 'clear_preset_var1xl' : clear_preset_var1xl,
                                 'clear_preset_var2' : clear_preset_var2,
                                 'clocked_on' : clocked_on,
                                 'next_state' : next_state,
                                 'preset' : preset
                                 }
      last_output_pins = output_pins
    elif isLatch:
      cell_library[cell_name] = {'comb': False,
                                 'delay': delay,
                                 'input': input_pins,
                                 'output': output_pins,
                                 'area' : cell_area,
                                 'leakage' : worstLeakege,
                                 'power': power,
                                 'clear' : clear,
                                 'data_in' : data_in,
                                 'enable' : enable
                                 }
      last_output_pins = output_pins


    input_pins = ""
    output_pins = {}
    delay = {}
    power = {}
  return cell_library

def write_to_json(cells, path_to_file):
  with open(path_to_file, 'w') as outfile:
    json.dump(cells, outfile, sort_keys=True, indent=4)

write_to_json(read_information_from_library(pathToLibraryFile), pathToJsonFile)
