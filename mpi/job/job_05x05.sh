#!/usr/bin/env bash

#SBATCH --job-name=mx_5x5
#SBATCH --output=out.5x5
#SBATCH --error=err.5x5
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=2
#SBATCH --ntasks=25
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1


WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 25 ./life_mpi 10 20000

