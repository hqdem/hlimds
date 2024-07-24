Available commands and their descriptions:

1. read_verilog [--frontend <frontend>] [--top <topModule>] [--debug]
   Description: Reads a Verilog design file.
   Parameters:
       --frontend <frontend>: Specifies the frontend to use (default: yosys).
       --top <topModule>: Specifies the top module (default: none).
       --debug: Enables debug mode (default: false).

2. write_design [--format <format>] [--path <fileName>]
   Description: Writes the current design to a file.
   Parameters:
       --format <format>: Specifies the output format (verilog, simple, dot) (default: verilog).
       --path <fileName>: Specifies the output file path (default: design.v).

3. verilog_to_fir [--out <outputFileName>] [--top <topModule>] [--debug]
   Description: Translate verilog to firrtl file.
   Parameters:
       --out <outputFileName>: Specifies the name to outupt file (default: out.fir).
       --top <topModule>: Specifies the top module (default: none).
       --debug: Enables debug mode (default: false).

4. lec [--method <method>]
   Description: Performs logical equivalence check.
   Parameters:
       --method <method>: Method for checking equivalence (bdd, fra, rnd, sat) (default: sat).

5. techmap [--type <mapperType>] [--area <area>] [--arrival <arrivalTime>]
   Description: Performs technology mapping.
   Parameters:
       --type <mapperType>: Type of mapper (af, area, delay, power) (default: area).

6. read_liberty <path>
   Description: Reads a liberty file for technology mapping.
   Parameters:
       <path>: Path to the liberty file.

7. write_subnet [--index <number>]
   Description: Writes a subnet.
   Parameters:
       --index <number>: Subnet sequence number (Outputs all subnets by default).

8. read_graphml <fileName>
   Description: Reads a GraphML design file.
   Parameters:
       <fileName>: Path to the GraphML file.

9. stats
   Description: Displays the parameters of the circuit.

10. clear
   Description: Clears the current design from memory.

11. pass <subcommand>
   Description: Applies optimization passes to the design.
   Subcommands:
       - aig: Mapping to the AIG representation.
       - mig: Mapping to the MIG representation.
       - b: Depth-aware balancing.
       - rw [--name <name>] [-k <k>] [-z]
           Description: Rewriting.
           Parameters:
               --name <name>: Name of the rewriting pass (default: rw).
               -k <k>: Parameter k for the rewriting pass (default: 4).
               -z: Enables zero-cost replacements (default: false).
       - rs [--name <name>] [-k <k>] [-n <n>]
           Description: Resubstitution.
           Parameters:
               --name <name>: Name of the resubstitution pass (default: rs).
               -k <k>: Cut size (default: 8).
               -n <n>: Cut size for calculating 'don't care' (default: 16).
       - rsz [--name <name>] [-k <k>] [-n <n>]
           Description: Resubstitution with zero-cost replacements.
           Parameters:
               --name <name>: Name of the resubstitution pass (default: rsz).
               -k <k>: Cut size (default: 8).
               -n <n>: Cut size for calculating 'don't care' (default: 16).
       - rwz
           Description: Rewrite zero-cost replacements.
       - rf
           Description: Refactor.
       - rfz
           Description: Refactor zero-cost replacements.
       - rfa
           Description: Refactor criterion area.
       - rfd
           Description: Refactor criterion delay.
       - rfp
           Description: Refactor criterion power.
       - ma
           Description: Technology Mapper criterion area.
       - md
           Description: Technology Mapper criterion delay.
       - mp
           Description: Technology Mapper criterion power.
       - resyn
           Description: Pre-defined script resyn. (b; rw; rwz; b; rwz; b)
       - resyn2
           Description: Pre-defined script resyn2. (b; rw; rf; b; rw; rwz; b; rfz; rwz; b)
       - resyn2a
           Description: Pre-defined script resyn2a. (b; rw; b; rw; rwz; b; rwz; b)
       - resyn3
           Description: Pre-defined script resyn3. (b; rs; rs -K 6; b; rsz; rsz -K 6; b; rsz -K 5; b)
       - compress
           Description: Pre-defined script compress. (b -l; rw -l; rwz -l; b -l; rwz -l; b -l)
       - compress2
           Description: Pre-defined script compress2. (b -l; rw -l; rf -l; b -l; rw -l; rwz -l; b -l; rfz -l; rwz -l; b -l)

12. show_time
   Description: Toggles the display of the passes execution time.

13. help
   Description: Shows the help.

14. exit
   Description: Closes the interactive shell mode.

15. dbstat [--db <dbpath>] [--otype <type>] [--out <outfile>] [--ttsize <ttsize>] <out-1> <out-2> ...
   Description: output information about the scheme (More info in `doc/DbStat.md`).
   Parameters:
       --db: path to database (default: none).
       --otype: type of output (default: BOTH).
       --out: path to output DOT file (default: none).
       --ttsize: number of inputs of truth table (default: none).
       <out-1> <out-2> ... <out-n> outputs of truth table.
