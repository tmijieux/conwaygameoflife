#!/usr/bin/env bash

#SBATCH --job-name=mx_12x12
#SBATCH --output=out.12x12
#SBATCH --error=err.12x12
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=8
#SBATCH --ntasks=144
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1

# 4 noeud / 1 proc mpi par noeud

WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 144 ./life_mpi 10 20004

