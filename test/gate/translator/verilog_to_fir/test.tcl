set inputfile [lindex $argv 0]
set outfile [lindex $argv 1]
if {[llength $argv] >= 3 && [lindex $argv 2] == "--verbose"} {
    verilog_to_fir --debug -o $outfile $inputfile
} else {
    verilog_to_fir -o $outfile $inputfile
}
