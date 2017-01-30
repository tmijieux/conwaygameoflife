#ifndef CGL_PROC_H
#define CGL_PROC_H

typedef struct cgl_proc_ cgl_proc;
#include <mpi.h>

struct cgl_proc_ {
    int rank;
    int group_size;
    int group_length;

    MPI_Comm cart_comm, line_comm, col_comm;
    int line, col;
    int prev_col, next_col;
    int prev_line, next_line;
};

void cgl_proc_init(cgl_proc *p);
void cgl_proc_fini(cgl_proc *P);

#endif // CGL_PROC_H
