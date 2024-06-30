set inputfile [lindex $argv 1]
set outfile [lindex $argv 2]
if {[llength $argv] >= 4 && [lindex $argv 3] == "--verbose"} {
    verilog_to_fir --debug -o $outfile $inputfile
} else {
    verilog_to_fir -o $outfile $inputfile
}
