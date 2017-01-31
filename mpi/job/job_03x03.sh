#!/usr/bin/env bash

#SBATCH --job-name=mx_3x3
#SBATCH --output=out.3x3
#SBATCH --error=err.3x3
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=1
#SBATCH --ntasks=9
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1


WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 9 ./life_mpi 10 20001

