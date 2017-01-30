#!/usr/bin/env bash

#SBATCH --job-name=mx_9x9
#SBATCH --output=out.9x9
#SBATCH --error=err.9x9
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=5
#SBATCH --ntasks=81
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1

# 4 noeud / 1 proc mpi par noeud

WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 81 ./life_mpi 10 20007

