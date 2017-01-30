#!/usr/bin/env bash
#SBATCH --job-name=mx_13x13
#SBATCH --output=out.13x13
#SBATCH --error=err.13x13
#SBATCH -p mistral
#SBATCH --time=02:00:00
#SBATCH --exclusive
#SBATCH --nodes=9
#SBATCH --ntasks=169
#SBATCH --ntasks-per-node=20
#SBATCH --cpus-per-task 1

# 4 noeud / 1 proc mpi par noeud

WORKDIR=${WORKDIR:-${HOME}/conwaygameoflife/mpi}
MPIEXEC=mpirun

cd ${WORKDIR}
. ./.module.load

${MPIEXEC} -np 169 ./life_mpi 10 20007
