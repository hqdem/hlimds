if {[llength $argv] >= 2} {
    set arg1 [lindex $argv 0]
    set arg2 [lindex $argv 1]
} else {
    puts "Not enough arguments provided, use:"
    puts "<path_to_grahml_file> <path_to_liberty_library>"
}
read_graphml $arg1
stat_design
logopt rw
stat_design
logopt rfd
stat_design
logopt rw
stat_design
read_liberty $arg2
techmap --type delay
stat_design
