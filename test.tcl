### Path to input circuits
set files [glob /workdir/utopia-eda/data_for_testing/*.graphml]
### Path to folder with resutls
set resultsPath log/

proc reopenStdout {file} {
    close stdout
    open $file w
}

proc testFile {file resultsPath} {
  set TIME_start [clock clicks -milliseconds]
  set s [split $file "/"]
  set s1 [lindex $s end]
  set ext ".txt"
  set comb $resultsPath$s1$ext
  set systemTime [clock seconds]
  puts -nonewline "[clock format $systemTime -format %D_%H:%M:%S]: "
  puts -nonewline $file...
  reopenStdout $comb

  read_graphml $file
  save_point p1
  stat_design
  logopt rf
  save_point p2
  lec p1 p2
  stat_design
  goto_point p1
  logopt myrf
  save_point p3
  lec p1 p3
  stat_design

  reopenStdout /dev/tty

  set TIME_taken [expr [clock clicks -milliseconds] - $TIME_start]
  puts "Complete ([expr $TIME_taken/1000.0] sec)"
}

file delete -force -- $resultsPath
file mkdir $resultsPath

foreach file $files {
  testFile $file $resultsPath
  
  reopenStdout /dev/null
  delete_design
  reopenStdout /dev/tty
}

puts "...All tests complete!..."
puts "Check $resultsPath for log files for each circuit."

exit