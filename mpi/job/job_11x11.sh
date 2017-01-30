#!/usr/bin/env bash

#SBATCH --job-name=mx_11x11
#SBATCH --output=out.11x11
#SBATCH --error=err.11x11
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=7
#SBATCH --ntasks=121
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1

# 4 noeud / 1 proc mpi par noeud

WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 121 ./life_mpi 10 20009

