$date    
    {{GEN_TIME}}$end
$version    
    0.0
$end
$timescale
    1ns
$end
$scope module {{NET_ID}} $end
{{#VARS}}$var wire 1 {{VAR_ID}} {{GID}} $end
{{/VARS}}$dumpall
$end
#0
$dumpvars
{{#VALUES}}{{VALUE}}{{VAR_ID}}
{{/VALUES}}$end
#1
