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

  for cell_group in library.get_groups('cell'):
    cell_name = (str(cell_group.args[0]))[1: -1]

    for pin_group in cell_group.get_groups('pin'):
      pin_name_attribute = str(pin_group.args[0])[1: -1]
      cell_area = cell_group['area']

      pin_direction_attribute = pin_group['direction']

      if pin_direction_attribute == "output":
        pin_function_attribute = str(pin_group["function"])[1: -1]
        output_pins[pin_name_attribute] = pin_function_attribute

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
                                'rise_transition': rise_transition_max_value
                                }

      else:
        input_pins += pin_name_attribute + " "

    if (("Q" not in output_pins)
        and ("CLK" not in input_pins) and (len(output_pins) != 0)):
      cell_library[cell_name] = {'delay': delay,
                                 'input': input_pins,
                                 'output': output_pins,
                                 'area' : cell_area
                                 }
      last_output_pins = output_pins

    input_pins = ""
    output_pins = {}
    delay = {}
    
  return cell_library

def write_to_json(cells, path_to_file):
  with open(path_to_file, 'w') as outfile:
    json.dump(cells, outfile, sort_keys=True, indent=4)

write_to_json(read_information_from_library(pathToLibraryFile), pathToJsonFile)
