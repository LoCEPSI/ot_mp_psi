# About The Repo

This repository hosts experimental code for "Practical Over-threshold Multiparty Private Set Intersection with Fast Online Execution," optimized for experimental use, not intended for production. It has been successfully compiled on Ubuntu 22.

## Quick Start

### Prerequisites

Ensure you have the following dependencies installed:
- [BOOST](https://www.boost.org/) (version 1.81)
- [GMP](https://gmplib.org/) (version 6.2.1)
- [NTL](https://libntl.org/) (version 11.5.1)

### Installation

You can easily set up your environment on Linux using the provided `install.sh` script:

```bash
sh ./install.sh
```
This script will automatically install all necessary dependencies and prepare the environment for the project.

### Building the Project

Once the dependencies are installed, you can build the project:
```bash
make clean
make all
```

##  Running  the Code

### Configuration

Before running the benchmarks, generate the necessary configuration files using gen_config.py. This script allows you to specify parameters for the experiments:

```bash
python ./tools/gen_config/gen_config.py 
```


Below are the command-line arguments you can use with the `gen_config.py` script to customize the setup of your experiment:

- `--set_size`: Size of the set (default: 64)
- `--false_positive_rate`: False positive rate (default: 10)
- `--number_of_parties`: Number of parties involved in the set intersection (default: 16)
- `--intersection_threshold`: Minimum number of parties agreeing for an item to be in the intersection (default: 10)
- `--benchmark_rounds`: Number of rounds to run the benchmark (default: 5)
- `--concurrency_level`: Number of threads for each party (default: 1)
- `--server_port`: Starting server port (default: 20081)
- `--no_print`: Suppress output printing (optional, action: store_true)

### Running a Single Experiment

To run a single experiment after setting up and building your project, execute the following command:

```bash
sh tools/run.sh
```

### Running Benchmarks

To execute a series of benchmarks to evaluate the performance of the system, use the following command:

```bash
sh tools/benchmark/benchmark.sh
```

### Benchmarking Over Different Parameters

Our project supports benchmarking over a variety of parameters to assess performance differences. This process can be easily managed through a custom script.


1. **Customize the Benchmark script**: Write your own script or modify our example to fit your needs. An example script can be found under `tools/benchmark/run_benchmark.sh`. This script illustrates how to set up and run benchmarks with different configurations.


2. **Execute the Script**: To run the benchmark, navigate to the script's directory and execute it:

```bash
   sh tools/benchmark/run_benchmark.sh
```

## Contact
For any inquiries, feel free to reach out:

-  hhe@mail.ustc.edu.cn
-  leyang@mail.ustc.edu.cn
