mkdir ../input
mkdir ../output
shopt -s nullglob
for v in *.v; do
    echo $v
    sed "s/\[\([0-9]*\)\]/_\1_/g" $v > ../input/$v
done
