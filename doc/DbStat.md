### Running DBStat 

`dbstat` is a utility that shows information about subnet from the database.
At the input, the utility receives the path to the serialized database and additional parameters. The utility expands the database instance in memory and outputs the required information about the desired subnet.

```console
dbstat --db <db-file> --otype < DOT | INFO | BOTH > --out <out-file> --ttsize <int> <out-1> <out-2> ...
```

Parameters of the command:

1. (Necessary) `<db-file>` path to the serialized database;
2. (Optional) `--otype` type of output.
   DOT - returns only .dot file,
   INFO - returns only information about subnet
   BOTH - returns both;
3. (Optional) `<out-file>` is a file to output DOT file. If the file name is not specified, it will output the contents to the console;
4. (Necessary) `--ttsize` specify number of input bits of Truth Table (TT).
5. (Necessary) (`<out-1>, <out-2> ... <out-n>`) sequences of values for outputs of the subnet, that encode outputs of it's truth table. Each sequence should be of $2^{ttsize}$ length, where ttsize is number of input bits. At least one output is required.
