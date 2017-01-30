#include <stdio.h>
#include <mpi.h>
#include "timer.h"

void cgl_timer_start(cgl_timer *T)
{
    perf(&T->p1);
}

void cgl_timer_stop(cgl_timer *T)
{
    perf(&T->p2);
    perf_diff(&T->p1, &T->p2);
}

void cgl_timer_print(cgl_timer *T, cgl_proc *P)
{
    uint64_t micro, max_t;

    micro = perf_get_micro(&T->p2);
    MPI_Reduce(&micro, &max_t, 1, MPI_UNSIGNED_LONG,
               MPI_MAX, 0, MPI_COMM_WORLD);
    if (!P->rank)
        printf("# %lu.%06lu s\n", max_t/1000000UL, max_t%1000000UL);
}
