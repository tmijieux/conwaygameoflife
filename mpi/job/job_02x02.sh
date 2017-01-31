#!/usr/bin/env bash

#SBATCH --job-name=mx_2x2
#SBATCH --output=out.2x2
#SBATCH --error=err.2x2
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1


WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 4 ./life_mpi 10 20000

