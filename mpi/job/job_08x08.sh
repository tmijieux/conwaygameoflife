#!/usr/bin/env bash

#SBATCH --job-name=mx_8x8
#SBATCH --output=out.8x8
#SBATCH --error=err.8x8
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=4
#SBATCH --ntasks=64
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1

# 4 noeud / 1 proc mpi par noeud

WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 64 ./life_mpi 10 20000

