#!/usr/bin/env bash

#SBATCH --job-name=mx_1x1
#SBATCH --output=out.1x1
#SBATCH --error=err.1x1
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1


WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 1 ./life_mpi 10 20000

