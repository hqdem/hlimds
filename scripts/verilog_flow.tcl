if {[llength $argv] >= 2} {
    set arg1 [lindex $argv 0]
    set arg2 [lindex $argv 1]
} else {
    puts "Not enough arguments provided, use:"
    puts "<path_to_verilog_file> <path_to_liberty_library>"
}
read_verilog $arg1
show_time
stats -l
pass rw
stats -l
pass rfa
stats -l
pass rw
stats -l
read_liberty $arg2
techmap --type area
stats
