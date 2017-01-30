#!/usr/bin/env bash

#SBATCH --job-name=mx_6x6
#SBATCH --output=out.6x6
#SBATCH --error=err.6x6
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=2
#SBATCH --ntasks=36
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1

# 4 noeud / 1 proc mpi par noeud

WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 36 ./life_mpi 10 20004

