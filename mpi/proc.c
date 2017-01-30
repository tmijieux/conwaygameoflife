
#include "util.h"
#include "proc.h"

void cgl_proc_init(cgl_proc *p)
{
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &p->group_size);

    double Nd = sqrt(p->group_size);
    ASSERT_MSG(
        "processus must be perfect integer square",
        floor(Nd) == Nd
    );

    int N = p->group_length = (int)Nd;
    int dims[2]    = {N, N};
    int periods[2] = {1, 1};

    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &p->cart_comm);
    MPI_Comm_rank(p->cart_comm, &p->rank);

    {
        int rem_dims[2] = {1, 0};
        MPI_Cart_sub(p->cart_comm, rem_dims, &p->line_comm);
        MPI_Comm_rank(p->line_comm, &p->col);
    }

    {
        int rem_dims[2] = {0, 1};
        MPI_Cart_sub(p->cart_comm, rem_dims, &p->col_comm);
        MPI_Comm_rank(p->col_comm, &p->line);
    }

    MPI_Cart_shift(p->line_comm, 0, 1, &p->prev_col, &p->next_col);
    MPI_Cart_shift(p->col_comm, 0, 1, &p->prev_line, &p->next_line);
}


void cgl_proc_fini(cgl_proc *P)
{
    memset(P, 0, sizeof*P);
    MPI_Finalize();
}
