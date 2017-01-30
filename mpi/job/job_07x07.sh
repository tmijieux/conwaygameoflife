#!/usr/bin/env bash

#SBATCH --job-name=mx_7x7
#SBATCH --output=out.7x7
#SBATCH --error=err.7x7
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=3
#SBATCH --ntasks=49
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1


WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 49 ./life_mpi 10 20006

