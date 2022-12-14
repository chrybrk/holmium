function status()
{
    if [[ $1 -eq 0 ]]; then
        echo -e "passed"
    else
        echo -e "failed"
    fi
}

project_name="Holmium"
project_bin_dir="./bin/holmium"
test_files=`ls test/*.ho`

echo "===================================="
echo "=            [ Holmium ]          =="
echo "===================================="
gcc -Isrc/include src/*.c -o $project_bin_dir/holmium
l=$?
echo -n "==> [INFO] compiling project: "
status $l
echo -e ""

for file in $test_files
do
    $project_bin_dir/holmium $file -o ./bin/output/$file.out
    l=$?
    echo -n "==> [TEST] file: $file, status: "
    status $l
done
