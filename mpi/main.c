#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "proc.h"
#include "timer.h"

static void print_reduce(cgl_proc *P, int num_alive)
{
    int total_num_alive;
    MPI_Reduce(&num_alive, &total_num_alive, 1,
               MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (P->rank == 0)
        printf("num_alive=%d\n", total_num_alive);
}

static void parse_cmdline(int argc, char **argv,
                          int *maxloop, int *board_size)
{
    if (argc < 3) {
        printf("Usage: %s Nb_Iterations Board_Size\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    *maxloop = atoi(argv[1]);    
    *board_size = atoi(argv[2]);
}

int main(int argc, char *argv[])
{
    cgl_proc P;
    cgl_board B;
    cgl_timer T;
    int maxloop, board_size;
    
    parse_cmdline(argc, argv, &maxloop, &board_size);
    cgl_proc_init(&P);

    if (!P.rank)
        printf("Running MPI version\n"
               "grid of size %d, %d iterations\n"
               "rank %d, group_size %d\n\n",
               board_size, maxloop, P.rank, P.group_size);

    cgl_board_init(&B, &P, board_size);

    cgl_timer_start(&T);
    int num_alive = cgl_board_main_loop(&B, &P, maxloop);
    cgl_timer_stop(&T);
    cgl_timer_print(&T, &P);
    print_reduce(&P, num_alive);
    
    cgl_proc_fini(&P);
    return EXIT_SUCCESS;
}
