
#!/bin/bash

# Set the desired values for the number_of_parties and intersection_threshold arguments
number_of_parties=(20)
intersection_threshold=(10)

# 2^-?
false_positive_rate=10

benchmark_rounds=1

concurrency_level=32


# Set the desired values for the set_size argument
set_sizes=(4)

# Set the output file
output_file="output/benchmark_output.txt"
> "$output_file"

# Loop over the different set sizes
for set_size in "${set_sizes[@]}"; do
    # Loop over the different number of parties and intersection thresholds
    for i in "${!number_of_parties[@]}"; do
        date
        # Generate the configuration files for the current set of parameters
        python3 ./tools/gen_config/gen_config.py --no_print --set_size "$set_size" --number_of_parties "${number_of_parties[$i]}" --intersection_threshold "${intersection_threshold[$i]}" --false_positive_rate "${false_positive_rate}" --benchmark_rounds "${benchmark_rounds}" --concurrency_level "${concurrency_level}" 
        # Run the benchmark using the generated configuration files, display the output on the command line, and save it to a file
        sh ./tools/benchmark/benchmark.sh | tee -a "$output_file"
    done
done