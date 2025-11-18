main_directory="testing"
output_naive="${main_directory}/naive"
output_optimised="${main_directory}/optimised"

main_directory_tuning="testing/save-tuning"
output_naive_tuning="${main_directory_tuning}/naive"
output_optimised_tuning="${main_directory_tuning}/optimised"

for i in $(seq 1 1 50); do
        output_file="${output_naive}/test${i}.json"
        clingo --outf=2 --quiet=1  "naive-encoding/instance.lp" "naive-encoding/encoding.lp" > "${output_file}"
done

for i in $(seq 1 1 50); do
        output_file="${output_optimised}/test${i}.json"
        clingo --outf=2 --quiet=1  "optimised_files/optimised_instances.lp" "optimised_files/optimised_second.lp" > "${output_file}"
done


echo "--save progress tests"

for i in $(seq 1 1 50); do
        output_file="${output_naive_tuning}/test${i}.json"
        clingo --outf=2 --quiet=1  "naive-encoding/instance.lp" "naive-encoding/encoding.lp" --save-progress > "${output_file}"
done

for i in $(seq 1 1 50); do
        output_file="${output_optimised_tuning}/test${i}.json"
        clingo --outf=2 --quiet=1  "optimised_files/optimised_instances.lp" "optimised_files/optimised_second.lp" --save-progress > "${output_file}"
done
