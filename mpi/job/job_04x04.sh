#!/usr/bin/env bash

#SBATCH --job-name=mx_4x4
#SBATCH --output=out.4x4
#SBATCH --error=err.4x4
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=1
#SBATCH --ntasks=16
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1


WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 16 ./life_mpi 10 20000

