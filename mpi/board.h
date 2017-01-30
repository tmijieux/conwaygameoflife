#ifndef CGL_BOARD_H
#define CGL_BOARD_H

#include <stdint.h>

typedef struct cgl_board_ cgl_board;

#include "proc.h"

#define CGL_BOARD_TYPE     uint8_t
#define CGL_BOARD_MPI_TYPE MPI_UNSIGNED_CHAR

#define cell( _i_, _j_ ) B->board[ B->ld_board * (_j_) + (_i_) ]
#define ngb( _i_, _j_ )  B->nb_neighbour[ B->ld_nb_neighbour * ((_j_) - 1) + ((_i_) - 1 ) ]


struct cgl_board_ {
    int n;
    CGL_BOARD_TYPE *board;
    uint8_t *nb_neighbour;
    int ld_board;
    int ld_nb_neighbour;
};

void cgl_board_init(cgl_board *B, cgl_proc *P, int64_t board_size);
int cgl_board_main_loop(cgl_board *B, cgl_proc *P, int maxloop);
void cgl_board_fini(cgl_board *B);

#endif // CGL_BOARD_H
