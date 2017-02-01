#!/usr/bin/env bash
#SBATCH --job-name=omp_diff_size
#SBATCH --output=out.omp
#SBATCH --error=err.omp
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=4
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task 20


do_job() {
    iter=$1
    size=$2
    ./life_omp $1 $2
}

for i in $(seq 100 100 1000); do
    do_job 10 $i
done

do_job 10 2500

for i in $(seq 5000 5000 20000); do
    do_job 10 $i
done
